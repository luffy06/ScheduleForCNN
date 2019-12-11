#include "theory_reschedule.h"

struct Scheduler {
  NodeGenerator ng;
  int launch_number;
  int period_count;
  long long total_time;

  Scheduler() { }

  Scheduler(int total_node, int pe_number, int round_limit, Node node_list[MAXN], int d) {
    ng = NodeGenerator(total_node, pe_number);
    launch_number = d;
    period_count = 0;
    total_time = 0;
  }

  void Show() {
    printf("Scheduler Launch:%d\tRound:%d\tTotalTime:%lld\n", launch_number, period_count, total_time);
    ng.Show();
  }
};

struct SchedulerComparator {
  bool operator()(Scheduler &a, Scheduler &b) const {
    if (a.total_time != b.total_time)
      return a.total_time + a.ng.period_time > b.total_time + b.ng.period_time;
    return a.ng.period_time / a.ng.period_round > b.ng.period_time / b.ng.period_round;
  }
};

bool CmpByTotalTime(Scheduler a, Scheduler b) {
  return a.total_time < b.total_time;
}

vector<Scheduler> schedulers;

void Init(int total_pe, int round_limit) {
  int pe_number = GetTopology();
  // printf("Multi:%d\n", pe_number);
  for (int i = 1; i <= total_pe / 2; ++ i)
    schedulers.push_back(Scheduler(total_node, i, round_limit, node_list, total_pe / i));
}

vector<Scheduler> SelectSchedulers(int total_pe, int total_rounds) {
  priority_queue<Scheduler, vector<Scheduler>, SchedulerComparator> q;
  vector<Scheduler> choosed_schedulers;
  int rest_pe = total_pe;

  for (int i = 0; i < schedulers.size(); ++ i) {
    Scheduler s = schedulers[i];
    schedulers[i].total_time = s.ng.prologue + Ceil(Ceil(total_rounds, s.launch_number), s.ng.period_round) * s.ng.period_time;
  }

  sort(schedulers.begin(), schedulers.end(), CmpByTotalTime);

  for (int i = 0; i < schedulers.size(); ++ i) {
    Scheduler s = schedulers[i];
    if (s.ng.pe_number > rest_pe)
      continue;
    s.launch_number = rest_pe / s.ng.pe_number;
    rest_pe = rest_pe - s.ng.pe_number * s.launch_number;
    s.total_time = s.ng.prologue;
    q.push(s);
  }

  // //           1  2  3  4  5  6
  // int ct[6] = {1, 0, 1, 0, 0, 0};
  // for (int i = 0; i < schedulers.size(); ++ i) {
  //   Scheduler s = schedulers[i];
  //   if (ct[i] > 0) {
  //     s.launch_number = ct[i];
  //     s.total_time = s.ng.prologue;
  //     q.push(s);
  //   }
  // }

  while (total_rounds > 0) {
    Scheduler s = q.top();
    q.pop();

    s.period_count = s.period_count + 1;
    s.total_time = s.total_time + s.ng.period_time;
    total_rounds = total_rounds - s.launch_number * s.ng.period_round;
    q.push(s);
  }

  while (!q.empty()) {
    Scheduler s = q.top();
    q.pop();
    if (s.period_count > 0)
      choosed_schedulers.push_back(s);
  }
  return choosed_schedulers;
}

FinalResult Solve(int total_pe, int total_rounds, int round_limit) {
  Init(total_pe, round_limit);
  for (int i = 0; i < schedulers.size(); ++ i) {
    memset(rechecked, false, sizeof(rechecked));
    vector<Edge> dram_edges;
    schedulers[i].ng.GeneratePeriodSchedule(round_limit, node_list);
    SpreadKeyNodeSet(schedulers[i].ng);
    ReUpdateRetiming(dram_edges, schedulers[i].ng);
    dram_edges = LoadInCache(schedulers[i].ng, "Dynamic");
    ReUpdateRetiming(dram_edges, schedulers[i].ng);
    schedulers[i].ng.CalcPrologue();
    #if TEST == 1
      printf("DRAM Edges For PE %d\n", schedulers[i].ng.pe_number);
      sort(dram_edges.begin(), dram_edges.end(), CmpEdge);
      for (int j = 0; j < dram_edges.size(); ++ j)
        dram_edges[j].Show();
    #endif
  }

  vector<Scheduler> choosed = SelectSchedulers(total_pe, total_rounds);
  #if TEST == 1
    for (int i = 0; i < choosed.size(); ++ i) {
      choosed[i].ng.ShowEach(false);
      choosed[i].ng.Show();
    }
  #endif



  FinalResult final_result = FinalResult();
  long long total_time = 0;
  double period_ratio = 0.;
  for (int i = 0; i < choosed.size(); ++ i) {
    Scheduler s = choosed[i];
    final_result.total_time = max(final_result.total_time, s.total_time);
    if (s.ng.prologue > final_result.prologue) {
      final_result.prologue = s.ng.prologue;
      final_result.retiming = s.ng.retiming;
    }
    final_result.run_on_cache_n = final_result.run_on_cache_n + s.ng.run_on_cache_n * s.period_count * s.launch_number;
    final_result.run_on_dram_n = final_result.run_on_dram_n + s.ng.run_on_dram_n * s.period_count * s.launch_number;
    final_result.run_on_cache = final_result.run_on_cache + s.ng.run_on_cache * s.period_count * s.launch_number;
    final_result.run_on_dram = final_result.run_on_dram + s.ng.run_on_dram * s.period_count * s.launch_number;
    period_ratio = period_ratio + s.ng.period_ratio;
  }
  final_result.cpu_ratio = (1.0 * total_rounds * total_cost) / (final_result.total_time * total_pe);
  final_result.period_ratio = period_ratio / choosed.size();
  return final_result;
}