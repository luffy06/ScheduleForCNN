struct IntermediateResult {
  int id;
  int from_id;
  int to_id;
  int from_round;
  int to_round;
  long long memory;
  long long start_time;
  long long end_time;

  bool valid;

  IntermediateResult(int f_id, int f_round, int t_id, int t_round, 
                      long long memory_cost, long long st, long long ed) {
    id = -1;
    from_id = f_id;
    to_id = t_id;
    from_round = f_round;
    to_round = t_round;
    memory = memory_cost;
    start_time = st;
    end_time = ed;
    valid = true;
    if (st >= ed)
      printf("%lld %lld\n", st, ed);
    assert(st < ed);
  }

  bool Equal(IntermediateResult ir) {
    if (from_id == ir.from_id && to_id == ir.to_id
      && from_round == ir.from_round && to_round == ir.to_round
      && memory == ir.memory && start_time == ir.start_time && end_time == ir.end_time)
      return true;
    return false;
  }

  void Show() {
    printf("Id:%d\tFrom:%d-%d\tTo:%d-%d\tST:%lld\tED:%lld\tMemory:%lld\tValid:%s\n", id, 
            from_id, from_round, to_id, to_round, start_time, end_time, memory, (valid ? "Valid" : "Invalid"));
  }
};

bool CmpByStartTime(IntermediateResult ira, IntermediateResult irb) {
  if (ira.start_time != irb.start_time)
    return ira.start_time < irb.start_time;
  else if (ira.end_time != irb.end_time)
    return ira.end_time < irb.end_time;
  return ira.id < irb.id;
}

struct CacheManager {
  int pe_id;
  int count_id;
  long long memory_sum;
  vector<IntermediateResult> all_results;
  set<long long> time_traces;
  
  CacheManager(int pe) {
    pe_id = pe;
    count_id = 0;
    memory_sum = 0;
    all_results.clear();
    time_traces.clear();
  }

  void AddIntermediateResult(IntermediateResult ir) {
    ir.id = count_id;
    count_id = count_id + 1;
    all_results.push_back(ir);
    memory_sum = memory_sum + ir.memory;
    time_traces.insert(ir.start_time);
    time_traces.insert(ir.end_time);
  }

  vector<long long> GetTimeTraces() {
    vector<long long> traces;
    for (set<long long>::iterator it = time_traces.begin(); it != time_traces.end(); ++ it)
      traces.push_back((*it));
    return traces;
  }

  void ShowAllResults() {
    printf("\nPE:%d\n", pe_id);
    for (int i = 0; i < all_results.size(); ++ i)
      all_results[i].Show();
  }

  vector<IntermediateResult> DetectCacheOverflow(string algo) {
    vector<IntermediateResult> put_in_dram;
    sort(all_results.begin(), all_results.end(), CmpByStartTime);
    vector<long long> traces = GetTimeTraces();
    for (int i = 0; i < (int)(traces.size()) - 1; ++ i) {
      long long st = traces[i];
      long long ed = traces[i + 1];

      vector<IntermediateResult> irs;
      vector<int> paddings;
      long long sum = 0;
      for (int j = 0; j < all_results.size(); ++ j) {
        if (all_results[j].start_time <= st && ed <= all_results[j].end_time && all_results[j].valid) {
          sum = sum + all_results[j].memory;
          irs.push_back(all_results[j]);
          paddings.push_back(all_results[j].memory);
        }
      }

      if (sum <= CACHESIZE)
        continue;

      vector<bool> choosed = ArrangeInFixedSize(paddings, CACHESIZE, algo);

      for (int k = 0; k < paddings.size(); ++ k) {
        if (!choosed[k]) {
          DeleteIntermediateResult(irs[k]);
          put_in_dram.push_back(irs[k]);
        }
      }
    }
    return put_in_dram;
  }


  void DeleteIntermediateResult(IntermediateResult ir) {
    for (int i = 0; i < all_results.size(); ++ i) {
      if (all_results[i].from_id == ir.from_id && 
          all_results[i].to_id == ir.to_id && 
          all_results[i].from_round == ir.from_round && 
          all_results[i].to_round == ir.to_round) {
        memory_sum = memory_sum - ir.memory;
        all_results[i].valid = false;
      }
    }
  }
};
