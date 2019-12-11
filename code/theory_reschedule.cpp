#include "theory_reschedule.h"

void Init(int total_pe, int round_limit) {
  int pe_number = GetTopology();

  if (total_pe >= pe_number) {
    ng_list.push_back(NodeGenerator(total_node, pe_number));
    if (total_pe % pe_number != 0)
      ng_list.push_back(NodeGenerator(total_node, total_pe % pe_number));
  }
  else {
    ng_list.push_back(NodeGenerator(total_node, total_pe));
  }
}

FinalResult Solve(int total_pe, int total_rounds, int round_limit) {
  Init(total_pe, round_limit);
  for (int i = 0; i < ng_list.size(); ++ i) {
    memset(rechecked, false, sizeof(rechecked));
    vector<Edge> dram_edges;
    ng_list[i].GeneratePeriodSchedule(round_limit, node_list);
    SpreadKeyNodeSet(ng_list[i]);
    ReUpdateRetiming(dram_edges, ng_list[i]);
    dram_edges = LoadInCache(ng_list[i], "Greedy");
    ReUpdateRetiming(dram_edges, ng_list[i]);
    ng_list[i].CalcPrologue();
    
    #if TEST == 1
      printf("DRAM Edges\n");
      sort(dram_edges.begin(), dram_edges.end(), CmpEdge);
      for (int j = 0; j < dram_edges.size(); ++ j)
        dram_edges[j].Show();
      ng_list[i].ShowEach(false);
      ng_list[i].Show();
    #endif
  }

  FinalResult final_result = CalcFinalResult(ng_list);
  return final_result;
}