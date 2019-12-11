#include "theory_lctes.h"

FinalResult Solve(int total_pe, int total_rounds, int round_limit) {
  int half_phase_round_limit = Init(total_pe);

  for (int i = 0; i < iter_list.size(); ++ i) {
    memset(rechecked, false, sizeof(rechecked));
    InitIteration(half_phase_round_limit, iter_list[i]);
    CalculateRetiming(iter_list[i]);
    vector<Edge> dram_edges = LoadInCache(iter_list[i], "Dynamic");
    ReUpdateRetiming(dram_edges, iter_list[i]);
    iter_list[i].CalcPrologue();
  }

  FinalResult final_result = CalcFinalResult(iter_list);
  return final_result;
}