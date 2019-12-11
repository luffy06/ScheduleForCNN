vector<NodeGenerator> ng_list;

void CalculateRetiming(NodeGenerator &ng) {
  queue<Node> q;
  for (int i = 0; i < ng.node_arr.size(); ++ i) {
    if (ng.node_arr[i].out_degree == 0) {
      q.push(ng.node_arr[i]);
    }
  }
  while (!q.empty()) {
    Node to_node = q.front();
    q.pop();
    to_node.certain = true;
    ng.SetNode(to_node);

    vector<Edge> pre_edges = re_edge_list[to_node.id];
    sort(pre_edges.begin(), pre_edges.end(), CmpEdgeByFromCost);
    for (int i = 0; i < pre_edges.size(); ++ i) {
      Edge e = pre_edges[i];
      long long cost = Ceil(e.memory, CACHESPEED);
      Node from_node = ng.GetNode(e.from, to_node.round);
      if (UpdatePrecursorRetiming(from_node, to_node, ng.period_time, cost))
        ng.SetNode(from_node);
      q.push(from_node);
    }
  }
  for (int i = 0; i < ng.node_arr.size(); ++ i) {
    if (!ng.node_arr[i].certain)
      ng.node_arr[i].Show();
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