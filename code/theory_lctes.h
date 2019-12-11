// REFERENCE https://dl.acm.org/citation.cfm?doid=3078633.3081032
const int PHASEROUNDLIMIT = 10;
vector<NodeGenerator> iter_list;

// topo_order:         small -> big
// max_out_edge - cost: big -> small
// id:                small -> big
struct NodeComparationByOutEdge {
  bool operator() (const Node &a, const Node &b) const {
    if (a.topo_order != b.topo_order)
      return a.topo_order > b.topo_order;
    else if (a.max_out_edge - a.cost != b.max_out_edge - b.cost)
      return a.max_out_edge - a.cost < b.max_out_edge - b.cost;
    return a.id > b.id;
  }
};

int CalcPhaseRound(int round_limit) {
  int phase_round = 0;
  int max_round = 0, min_round = INF;
  for (int i = 1; i <= total_node; ++ i) {
    long long com_sum = 0;
    int count = 0;
    vector<Edge> edges = edge_list[node_list[i].id];
    for (int j = 0; j < edges.size(); ++ j) {
      Edge e = edges[j];
      if (Ceil(e.memory, CACHESPEED) > 0) {
        com_sum = com_sum + Ceil(e.memory, CACHESPEED);
        count = count + 1;
      }
    }
    if (count == 0)
      continue;
    int cur_round = Ceil(com_sum, (1LL * count * node_list[i].cost));
    max_round = max(max_round, cur_round);
    min_round = min(min_round, cur_round);
  }
  if (max_round == 0)
    phase_round = 2;
  else
    phase_round = (max_round + min_round) / 2;
  phase_round = min(phase_round, round_limit);
  phase_round = max(phase_round, 2);
  return phase_round;
}

int Init(int total_pe) {
  int pe_number = GetTopology();
  sort(node_list + 1, node_list + 1 + total_node, CmpByTopoOrder);

  int half_phase_round_limit = CalcPhaseRound(PHASEROUNDLIMIT);
  // the PE number of iteration is one bigger than phase
  pe_number = pe_number + 1;
  if (total_pe >= pe_number) {
    iter_list.push_back(NodeGenerator(total_node, pe_number));
    if (total_pe % pe_number > 0)
      iter_list.push_back(NodeGenerator(total_node, total_pe % pe_number));
  }
  else {
    iter_list.push_back(NodeGenerator(total_node, total_pe));
  }
  return half_phase_round_limit;
}

void InitHalfPhaseWithPriority(NodeGenerator &phase) {
  priority_queue<TimeInterval> interval_que;
  priority_queue<Node, vector<Node>, NodeComparationByOutEdge> node_que;

  phase.period_time = total_cost * phase.period_round;
  
  for (int i = 1; i <= phase.pe_number; ++ i)
    interval_que.push(TimeInterval(i, 0, 0));

  int index = 1;
  int cur_layer = -1;
  long long max_end_time = 0;
  do {
    queue<TimeInterval> temp_que;
    while (!interval_que.empty()) {
      TimeInterval ti = interval_que.top();
      interval_que.pop();
      ti.SetTime(ti.start_time, max_end_time);
      temp_que.push(ti);
    }

    while (!temp_que.empty()) {
      TimeInterval ti = temp_que.front();
      temp_que.pop();
      interval_que.push(ti);      
    }

    for (; index <= total_node && node_list[index].layer == cur_layer; ++ index)
      node_que.push(node_list[index]);

    while (!node_que.empty()) {
      Node node = node_que.top();
      node_que.pop();
      TimeInterval ti = interval_que.top();
      interval_que.pop();
      for (int k = 1; k <= phase.period_round; ++ k) {
        node.round = k;
        node.pe_id = ti.pe_id;
        node.SetTime(ti.end_time, ti.end_time + node.cost);
        phase.SetNode(node);

        ti.SetTime(ti.start_time, node.end_time);
        max_end_time = max(max_end_time, node.end_time);
      }
      interval_que.push(ti);
    }

    if (index <= total_node)
      cur_layer = node_list[index].layer;
  } while (index <= total_node);

  phase.period_time = max_end_time;
  phase.period_ratio = total_cost * phase.period_round / (1.0 * phase.period_time * phase.pe_number);
}

void InitIteration(int half_phase_round_limit, NodeGenerator &iteration) {
  int max_ratio_round = 1;
  double max_phase_ratio = 0.;
  int iteration_pe_number = iteration.pe_number;
  // 初始化半个phase
  if (iteration_pe_number > 1)
    iteration = NodeGenerator(total_node, iteration_pe_number - 1);
  else
    iteration = NodeGenerator(total_node, iteration_pe_number);

  for (int i = 1; i <= half_phase_round_limit; ++ i) {
    iteration.AddOneRoundNodes(node_list);
    iteration.Reset();
    InitHalfPhaseWithPriority(iteration);
    if (iteration.period_ratio > max_phase_ratio) {
      max_phase_ratio = iteration.period_ratio;
      max_ratio_round = i;
    }
    if (i > 1 && max_phase_ratio >= LIMITEDRATIO)
      break;
  }
  if (max_phase_ratio < LIMITEDRATIO) {
    if (iteration_pe_number > 1)
      iteration = NodeGenerator(total_node, iteration_pe_number - 1);
    else
      iteration = NodeGenerator(total_node, iteration_pe_number);
    for (int i = 0; i < max_ratio_round; ++ i)
      iteration.AddOneRoundNodes(node_list);
    InitHalfPhaseWithPriority(iteration);
  }
  int phase_pe_number = iteration.pe_number;
  iteration.pe_number = iteration_pe_number;

  // 旋转半个phase构成一整个phase
  // 1. 获取半个phase和另外半个phase的PE映射表
  int shift_pe_number = iteration.pe_number - phase_pe_number;
  vector<int> shift_map;
  vector<long long> shift_distance;
  vector<long long> pe_end_times;
  vector<long long> pe_start_times;
  shift_map.push_back(-1);
  shift_distance.push_back(0);
  pe_start_times.push_back(iteration.period_time);
  pe_end_times.push_back(0);
  for (int i = 1; i <= iteration.pe_number; ++ i) {
    int map_pe_id = phase_pe_number - i + shift_pe_number + 1;
    if (i <= shift_pe_number)
      map_pe_id = -1;
    shift_map.push_back(map_pe_id);
    shift_distance.push_back(0);
    pe_start_times.push_back(iteration.period_time);
    pe_end_times.push_back(0);
  }

  for (int i = 1; i <= iteration.node_arr.size(); ++ i) {
    Node node = iteration.node_arr[i];
    int pe_id = node.pe_id;
    pe_end_times[pe_id] = max(pe_end_times[pe_id], node.end_time);
    pe_start_times[pe_id] = min(pe_start_times[pe_id], node.start_time);
  }

  // 2. 计算另外半个phase对应PE的向前缩进距离
  for (int i = shift_pe_number + 1; i <= iteration.pe_number; ++ i) {
    int map_pe_id = shift_map[i];
    if (i <= phase_pe_number) {
      shift_distance[i] = pe_end_times[i] - pe_start_times[map_pe_id];
      long long pe_end_time = pe_end_times[map_pe_id] + shift_distance[i];
      iteration.period_time = max(iteration.period_time, pe_end_time);
    }
    else {
      assert(iteration.period_time >= pe_end_times[map_pe_id]);
      shift_distance[i] = iteration.period_time - pe_end_times[map_pe_id];
    }
  }

  // 3. 添加另外半个phase
  int size_ = iteration.node_arr.size();
  for (int i = 0; i < size_; ++ i) {
    Node node = iteration.node_arr[i];
    int map_pe_id_rev = -1;
    for (int j = 1; j <= iteration.pe_number; ++ j)
      if (shift_map[j] == node.pe_id)
        map_pe_id_rev = j;
    assert(map_pe_id_rev != -1);
    node.pe_id = map_pe_id_rev;
    node.start_time = node.start_time + shift_distance[map_pe_id_rev];
    node.end_time = node.end_time + shift_distance[map_pe_id_rev];
    node.round = node.round + iteration.period_round;
    iteration.node_arr.push_back(node);
  }
  iteration.period_round = iteration.period_round * 2;
  sort(iteration.node_arr.begin(), iteration.node_arr.end(), CmpById);
}

void CalculateRetiming(NodeGenerator &iteration) {
  queue<Node> q;
  for (int i = 0; i < iteration.node_arr.size(); ++ i) {
    if (iteration.node_arr[i].out_degree == 0) {
      q.push(iteration.node_arr[i]);
    }
  }
  while (!q.empty()) {
    Node to_node = q.front();
    q.pop();
    to_node.certain = true;
    iteration.SetNode(to_node);

    vector<Edge> pre_edges = re_edge_list[to_node.id];
    sort(pre_edges.begin(), pre_edges.end(), CmpEdgeByFromCost);
    for (int i = 0; i < pre_edges.size(); ++ i) {
      Edge e = pre_edges[i];
      long long cost = Ceil(e.memory, CACHESPEED);
      Node from_node = iteration.GetNode(e.from, to_node.round);
      if (UpdatePrecursorRetiming(from_node, to_node, iteration.period_time, cost))
        iteration.SetNode(from_node);
      q.push(from_node);
    }
  }
  for (int i = 0; i < iteration.node_arr.size(); ++ i) {
    if (!iteration.node_arr[i].certain)
      iteration.node_arr[i].Show();
  }
}

vector<Edge> LoadInCache(NodeGenerator &iteration, string algo) {
  memset(visited, false, sizeof(visited));
  vector<CacheManager> cache_managers;
  for (int i = 1; i <= iteration.pe_number; ++ i)
    cache_managers.push_back(CacheManager(i));

  queue<Node> q;
  for (int i = 0; i < iteration.node_arr.size(); ++ i) {
    if (iteration.node_arr[i].topo_order == 0) {
      visited[iteration.node_arr[i].id][iteration.node_arr[i].round] = true;
      q.push(iteration.node_arr[i]);
    }
  }

  while (!q.empty()) {
    Node from_node = q.front();
    q.pop();

    vector<Edge> suf_edges = edge_list[from_node.id];
    for (int i = 0; i < suf_edges.size(); ++ i) {
      Edge e = suf_edges[i];
      Node to_node = iteration.GetNode(e.to, from_node.round);
      iteration.run_on_cache = iteration.run_on_cache + e.memory;
      iteration.run_on_cache_n = iteration.run_on_cache_n + 1;

      // 单周期内检查内存溢出
      if (from_node.retiming == to_node.retiming) {
        if (from_node.end_time >= to_node.start_time) {
          printf("### Wrong Retiming ###\n");
          from_node.Show();
          to_node.Show();
        }

        IntermediateResult ir = IntermediateResult(from_node.id, from_node.round, 
                                                    to_node.id, to_node.round, 
                                                    e.memory, 
                                                    from_node.end_time, 
                                                    to_node.start_time);
        cache_managers[to_node.pe_id - 1].AddIntermediateResult(ir);
      }
      else {
        if (from_node.end_time < iteration.period_time) {
          IntermediateResult ir = IntermediateResult(from_node.id, from_node.round, 
                                                      to_node.id, to_node.round, 
                                                      e.memory, from_node.end_time, 
                                                      iteration.period_time);
          cache_managers[to_node.pe_id - 1].AddIntermediateResult(ir);
        }
        if (to_node.start_time > 0) {
          IntermediateResult ir = IntermediateResult(from_node.id, from_node.round, 
                                                      to_node.id, to_node.round, 
                                                      e.memory, 
                                                      0, 
                                                      to_node.start_time);
          cache_managers[to_node.pe_id - 1].AddIntermediateResult(ir);
        }
        if (to_node.retiming - from_node.retiming > 1) {
          for (int i = 0; i < to_node.retiming - from_node.retiming - 1; ++ i) {
            IntermediateResult ir = IntermediateResult(from_node.id, from_node.round, 
                                                        to_node.id, to_node.round, 
                                                        e.memory, 
                                                        0, 
                                                        iteration.period_time);
            cache_managers[to_node.pe_id - 1].AddIntermediateResult(ir);            
          }
        }
      }
      if (!visited[to_node.id][to_node.round]) {
        visited[to_node.id][to_node.round] = true;
        q.push(to_node);
      }
    }
  }

  vector<Edge> dram_edges;
  for (int i = 0; i < iteration.pe_number; ++ i) {
    vector<IntermediateResult> put_in_dram = cache_managers[i].DetectCacheOverflow(algo);
    for (int j = 0; j < put_in_dram.size(); ++ j) {
      IntermediateResult ir = put_in_dram[j];
      if (rechecked[ir.to_id][ir.to_round] == false) {
        iteration.run_on_cache = iteration.run_on_cache - ir.memory;
        iteration.run_on_cache_n = iteration.run_on_cache_n - 1;
        iteration.run_on_dram = iteration.run_on_dram + ir.memory;
        iteration.run_on_dram_n = iteration.run_on_dram_n + 1;
      }
      rechecked[ir.to_id][ir.to_round] = true;
      dram_edges.push_back(Edge(ir.from_id, ir.to_id, ir.from_round, ir.memory));
    }
  }
  return dram_edges;
}

bool StoreInCache(int from_id, int to_id, int round, vector<Edge> dram_edges) {
  for (int i = 0; i < dram_edges.size(); ++ i) {
    Edge e = dram_edges[i];
    if (e.from == from_id && e.to == to_id && e.round == round) 
      return false;
  }
  return true;
}

void ReBFS(Node start_node, vector<Edge> dram_edges, NodeGenerator &iteration) {
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  q.push(start_node);
  while (!q.empty()) {
    Node to_node = q.top();
    q.pop();
    rechecked[to_node.id][to_node.round] = false;

    vector<Edge> pre_edges = re_edge_list[to_node.id];
    sort(pre_edges.begin(), pre_edges.end(), CmpEdgeByFromCost);
    for (int i = 0; i < pre_edges.size(); ++ i) {
      Edge e = pre_edges[i];
      Node from_node = iteration.GetNode(pre_edges[i].from, to_node.round);
      long long cost = (StoreInCache(from_node.id, to_node.id, to_node.round, dram_edges) 
                      ? Ceil(e.memory, CACHESPEED) : Ceil(e.memory, DRAMSPEED));
      if (UpdatePrecursorRetiming(from_node, to_node, iteration.period_time, cost)) {
        iteration.SetNode(from_node);
        q.push(from_node);
      }
    }
  }
}

void ReUpdateRetiming(vector<Edge> dram_edges, NodeGenerator &iteration) {
  vector<Node> rechecked_node_list;
  for (int i = 1; i <= total_node; ++ i)
    for (int j = 1; j <= iteration.period_round; ++ j)
      if (rechecked[i][j]) rechecked_node_list.push_back(iteration.GetNode(i, j));
  
  sort(rechecked_node_list.begin(), rechecked_node_list.end(), CmpByTopoOrder);

  for (int i = (int)(rechecked_node_list.size()) - 1; i >= 0; -- i) {
    Node start_node = rechecked_node_list[i];
    if (rechecked[start_node.id][start_node.round])
      ReBFS(iteration.GetNode(start_node.id, start_node.round), dram_edges, iteration);
  }
}