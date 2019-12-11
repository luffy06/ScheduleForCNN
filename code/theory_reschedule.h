vector<NodeGenerator> ng_list;
vector<PEInterval> pe_intervals[MAXPE];

void ShowInterval(int pe_id) {
  printf("pe_id:%d\n", pe_id);
  if (pe_intervals[pe_id].size() == 0) {
    printf("None Interval\n");
    return;
  }
  for (int i = 0; i < pe_intervals[pe_id].size(); ++ i) {
    printf("[%lld, %lld]\n", pe_intervals[pe_id][i].start_time, pe_intervals[pe_id][i].end_time);
  }
}

vector<Node> GetKeyNodeSet(NodeGenerator &ng) {
  vector<Node> uncertain_node_set;
  vector<Node> key_node_set;
  long long max_cost = -1;
  for (int i = 1; i <= total_node; ++ i) {
    for (int j = 1; j <= ng.period_round; ++ j) {
      Node node = ng.GetNode(i, j);
      if (!node.certain) {
        uncertain_node_set.push_back(node);
        max_cost = max(max_cost, node.cost);
      }
    }
  }

  for (int i = 0; i < uncertain_node_set.size(); ++ i)
    if (uncertain_node_set[i].cost >= (int)(max_cost * ALPHA))
      key_node_set.push_back(uncertain_node_set[i]);

  sort(key_node_set.begin(), key_node_set.end(), CmpByCost);
  return key_node_set;
}

void SplitInterval(PEInterval pe_int, int start_time, int end_time) {
  int pe_id = pe_int.pe_id;
  if (pe_int.start_time < start_time)
    pe_intervals[pe_id].push_back(PEInterval(pe_id, pe_int.start_time, start_time));
  if (end_time < pe_int.end_time)
    pe_intervals[pe_id].push_back(PEInterval(pe_id, end_time, pe_int.end_time));
  sort(pe_intervals[pe_id].begin(), pe_intervals[pe_id].end());
}

void PlaceKeyNode(Node &key_node, NodeGenerator &ng) {
  // 找到关键节点所在的活动区间
  int interval_index = -1;
  PEInterval pe_int;
  for (int i = 0; i < pe_intervals[key_node.pe_id].size(); ++ i) {
    pe_int = pe_intervals[key_node.pe_id][i];
    if (!pe_int.valid)
      continue;
    if (pe_int.start_time <= key_node.start_time && key_node.end_time <= pe_int.end_time) {
      interval_index = i;
      pe_intervals[key_node.pe_id][i].valid = false;
      break;
    }
  }

  if (interval_index == -1) {
    key_node.Show();
    ShowInterval(key_node.pe_id);
    printf("### Not Found key_node Interval ###\n");
  }
  assert(interval_index != -1);
  
  // 更新关键节点信息
  key_node.start_time = pe_intervals[key_node.pe_id][interval_index].start_time;
  key_node.end_time = key_node.start_time + key_node.cost;
  key_node.certain = true;
  ng.SetNode(key_node);
  
  // 更新同活动区间内其他节点的区间信息
  vector<Node> uncertain_node_set = ng.GetSamePEOtherNodes(key_node.id, key_node.round, pe_int);
  for (int k = 0; k < uncertain_node_set.size(); ++ k) {
    uncertain_node_set[k].SetTime(key_node.end_time, pe_int.end_time);
    ng.SetNode(uncertain_node_set[k]);
  }

  // 分裂活动区间
  SplitInterval(pe_int, key_node.start_time, key_node.end_time);  
}

Node ReallocatePrecursorNode(Node from_node, Node to_node, long long cost, NodeGenerator &ng) {
  long long min_end_time = to_node.start_time + to_node.retiming * ng.period_time - cost;
  long long min_start_time = min_end_time - from_node.cost;

  if (!from_node.certain) {
    // 前继节点为未确定态
    // 获取前继节点所在的活动区间
    int interval_index = -1;
    int pe_id = from_node.pe_id;
    PEInterval pe_int;
    for (int i = 0; i < pe_intervals[pe_id].size(); ++ i) {
      pe_int = pe_intervals[pe_id][i];
      if (!pe_int.valid)
        continue;
      if (pe_int.start_time <= from_node.start_time 
        && pe_int.end_time >= from_node.end_time) {
        interval_index = i;
        pe_intervals[pe_id][i].valid = false;
        break;
      }
    }

    if (interval_index == -1) {
      from_node.Show();
      ShowInterval(pe_id);
    }
    assert(interval_index != -1);

    // PS + retiming * P <= S
    from_node.retiming = Floor(min_start_time - pe_int.start_time, ng.period_time);
    min_end_time = min(pe_int.end_time + from_node.retiming * ng.period_time, min_end_time);
    min_start_time = min_end_time - from_node.cost;

    long long bin_size = min_start_time - (pe_int.start_time + from_node.retiming * ng.period_time);
    vector<Node> uncertain_node_set = ng.GetSamePEOtherNodes(from_node.id, 
                                                            from_node.round, 
                                                            pe_int);
    vector<int> paddings;
    for (int i = 0; i < uncertain_node_set.size(); ++ i)
      paddings.push_back(uncertain_node_set[i].cost);
    vector<bool> choosed = ArrangeInFixedSize(paddings, bin_size, "Greedy");
    long long cost_sum = 0;
    for (int i = 0; i < paddings.size(); ++ i)
      if (choosed[i])
        cost_sum = cost_sum + paddings[i];
    assert(cost_sum <= bin_size);
    
    from_node.SetTime(pe_int.start_time + cost_sum, pe_int.start_time + cost_sum + from_node.cost);

    for (int i = 0; i < uncertain_node_set.size(); ++ i) {
      if (choosed[i]) {
        Node uncertain_node = uncertain_node_set[i];
        uncertain_node.SetTime(pe_int.start_time, from_node.start_time);
        ng.SetNode(uncertain_node);
      }
    }

    for (int i = 0; i < uncertain_node_set.size(); ++ i) {
      if (!choosed[i]) {
        Node uncertain_node = uncertain_node_set[i];
        uncertain_node.SetTime(from_node.end_time, pe_int.end_time);
        ng.SetNode(uncertain_node);
      }
    }
    SplitInterval(pe_int, from_node.start_time, from_node.end_time);
  }
  else {
    // 前继节点为已确定态，更新Retiming值，设置为待检查态
    if (from_node.start_time + from_node.retiming * ng.period_time > min_start_time)
      from_node.retiming = Floor(min_start_time - from_node.start_time, ng.period_time);
  }
  return from_node;
}

void BFS(Node key_node, NodeGenerator &ng) {
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  q.push(key_node);
  while (!q.empty()) {
    Node to_node = q.top();
    q.pop();
    
    vector<Edge> pre_edges = re_edge_list[to_node.id];
    sort(pre_edges.begin(), pre_edges.end(), CmpEdgeByFromCost);
    for (int i = 0; i < pre_edges.size(); ++ i) {
      Edge e = pre_edges[i];
      // long long Com = (PEEdge[to_node.pe_id][from_node.pe_id] == 0 ? 0 : Ceil(memory, PEEdge[to_node.pe_id][from_node.pe_id]));
      long long cost = Ceil(e.memory, CACHESPEED);
      Node from_node = ReallocatePrecursorNode(ng.GetNode(e.from, to_node.round), to_node, cost, ng);

      if (!from_node.certain)
        q.push(from_node);
      else
        rechecked[from_node.id][from_node.round] = true;
      from_node.certain = true;
      ng.SetNode(from_node);
    }
  }
}

void SpreadKeyNodeSet(NodeGenerator &ng) {
  for (int i = 1; i <= ng.pe_number; ++ i) {
    pe_intervals[i].clear();
    pe_intervals[i].push_back(PEInterval(i, 0, ng.period_time));
  }

  while (true) {
    vector<Node> key_node_set = GetKeyNodeSet(ng);
    if (key_node_set.size() == 0)
      break;
    for (int i = 0; i < key_node_set.size(); ++ i) {
      Node key_node = ng.GetNode(key_node_set[i].id, key_node_set[i].round);
      if (!key_node.certain) { // if !checked
        PlaceKeyNode(key_node, ng);
        BFS(key_node, ng);
      }
    }
  }
}

vector<Edge> LoadInCache(NodeGenerator &ng, string algo) {
  memset(visited, false, sizeof(visited));
  vector<CacheManager> cache_managers;
  for (int i = 1; i <= ng.pe_number; ++ i)
    cache_managers.push_back(CacheManager(i));

  queue<Node> q;
  for (int i = 0; i < ng.node_arr.size(); ++ i) {
    if (ng.node_arr[i].topo_order == 0) {
      visited[ng.node_arr[i].id][ng.node_arr[i].round] = true;
      q.push(ng.node_arr[i]);
    }
  }

  while (!q.empty()) {
    Node from_node = q.front();
    q.pop();

    vector<Edge> suf_edges = edge_list[from_node.id];
    for (int i = 0; i < suf_edges.size(); ++ i) {
      Edge e = suf_edges[i];
      Node to_node = ng.GetNode(e.to, from_node.round);
      ng.run_on_cache = ng.run_on_cache + e.memory;
      ng.run_on_cache_n = ng.run_on_cache_n + 1;

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
        if (from_node.end_time < ng.period_time) {
          IntermediateResult ir = IntermediateResult(from_node.id, from_node.round, 
                                                      to_node.id, to_node.round, 
                                                      e.memory, from_node.end_time, 
                                                      ng.period_time);
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
                                                        ng.period_time);
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
  for (int i = 0; i < ng.pe_number; ++ i) {
    vector<IntermediateResult> put_in_dram = cache_managers[i].DetectCacheOverflow(algo);
    for (int j = 0; j < put_in_dram.size(); ++ j) {
      IntermediateResult ir = put_in_dram[j];
      if (rechecked[ir.to_id][ir.to_round] == false) {
        ng.run_on_cache = ng.run_on_cache - ir.memory;
        ng.run_on_cache_n = ng.run_on_cache_n - 1;
        ng.run_on_dram = ng.run_on_dram + ir.memory;
        ng.run_on_dram_n = ng.run_on_dram_n + 1;
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

void ReBFS(Node start_node, vector<Edge> dram_edges, NodeGenerator &ng) {
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
      Node from_node = ng.GetNode(pre_edges[i].from, to_node.round);

      long long cost = (StoreInCache(from_node.id, to_node.id, to_node.round, dram_edges) 
                      ? Ceil(e.memory, CACHESPEED) : Ceil(e.memory, DRAMSPEED));
      if (UpdatePrecursorRetiming(from_node, to_node, ng.period_time, cost)) {
        ng.SetNode(from_node);
        q.push(from_node);
      }
    }
  }
}

void ReUpdateRetiming(vector<Edge> dram_edges, NodeGenerator &ng) {
  vector<Node> rechecked_node_list;
  for (int i = 1; i <= total_node; ++ i)
    for (int j = 1; j <= ng.period_round; ++ j)
      if (rechecked[i][j]) rechecked_node_list.push_back(ng.GetNode(i, j));

  sort(rechecked_node_list.begin(), rechecked_node_list.end(), CmpByTopoOrder);

  for (int i = (int)(rechecked_node_list.size()) - 1; i >= 0; -- i) {
    Node start_node = rechecked_node_list[i];
    if (rechecked[start_node.id][start_node.round])
      ReBFS(ng.GetNode(start_node.id, start_node.round), dram_edges, ng);
  }
}