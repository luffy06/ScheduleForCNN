#define MAXM 20000
#define MAXN 8000
#define MAXSIZE 5000
#define MAXPE 300
#define MINR 55
#define MAXR 6005
#define LIMITEDRATIO 0.90
#define ALPHA 0.8
const int INF = 0x3f3f3f3f;
const long long LLINF = 0x3f3f3f3f3f3f3f3f;
#if TEST == 0
  const long long DRAMSPEED = 10000;
  const long long CACHESPEED = 100000;
  const long long CACHESIZE = 20480;
#else
  const long long DRAMSPEED = 1;
  const long long CACHESPEED = 2;
  const long long CACHESIZE = 2;
#endif

typedef pair<int, int> TwoInt;

void TestInput() {
  int t;
  scanf("%d", &t);
}

long long Ceil(long long a, long long b) {
  if (a % b == 0)
    return a / b;
  if (a > 0)
    return a / b + 1;
  return a / b;
}

long long Floor(long long a, long long b) {
  if (a % b == 0)
    return a / b;
  if (a > 0)
    return a / b;
  return a / b - 1;
}

long long GetTime() {
  std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()
  );
  return ms.count();
}

struct FinalResult {
  long long total_time;
  long long prologue;
  long long period_time;
  int retiming;
  long long run_on_cache_n;
  long long run_on_dram_n;
  long long run_on_cache;
  long long run_on_dram;
  double cache_ratio;
  double period_ratio;
  double cpu_ratio;

  FinalResult() {
    total_time = -1;
    period_time = 0;
    prologue = 0;
    retiming = 0;
    run_on_cache_n = 0;
    run_on_dram_n = 0;
    run_on_cache = 0;
    run_on_dram = 0;
    cache_ratio = 0;
    period_ratio = 0;
    cpu_ratio = 0;
  }

  void Show(int total_node, int total_edge) {
    printf("TotalNode:%d\nTotalEdge:%d\nPeriodTime:%lld\nTotalTime:%lld\nPrologue:%lld\nRetiming:%d\nRunOnCacheN:%lld\nRunOnDRAMN:%lld\nRunOnCache:%lld\nRunOnDRAM:%lld\nPeriodRatio:%.6f\nCPURatio:%.6f\n", 
            total_node, total_edge, period_time, total_time, prologue, retiming, run_on_cache_n, run_on_dram_n, run_on_cache, run_on_dram, period_ratio, cpu_ratio);
  }
};

struct Node {
  // Constant Attributes
  int id;
  long long cost;
  char name[200];
  int layer;
  int in_degree;
  int out_degree;
  int topo_order;

  // Variant Attributes
  int pe_id;
  int round;
  int retiming;

  long long start_time;
  long long end_time;

  bool certain;
  long long max_out_edge;
  int pe_numb;

  Node() {
    id = -1;
    cost = 0;
    layer = -1;
    in_degree = out_degree = 0;
    topo_order = -1;
    pe_id = -1;
    round = -1;
    retiming = 0;
    start_time = end_time = -1;
    max_out_edge = 0;
    certain = false;
    pe_numb = -1;
  }

  Node(int a, long long b) {
    id = a;
    cost = b;
    layer = -1;
    in_degree = out_degree = 0;
    topo_order = -1;
    pe_id = -1;
    round = -1;
    retiming = 0;
    start_time = end_time = -1;
    max_out_edge = 0;
    certain = false;
    pe_numb = -1;
  }

  void SetTime(long long st, long long ed) {
    start_time = st;
    end_time = ed;
    assert(start_time <= end_time);
  }

  void Copy(Node t) {
    id = t.id;
    cost = t.cost;
    strcpy(name, t.name);
    layer = t.layer;
    in_degree = t.in_degree;
    out_degree = t.out_degree;
    topo_order = t.topo_order;
    pe_id = t.pe_id;
    round = t.round;
    retiming = t.retiming;
    start_time = t.start_time;
    end_time = t.end_time;
    max_out_edge = t.max_out_edge;
    certain = t.certain;
    pe_numb = t.pe_numb;
  }

  void Show() {
    printf("ID:%d\tCost:%lld\tLayer:%d\tInDe:%d\tOutDe:%d\tTopo:%d\tPEId:%d\tRound:%d\tRetiming:%d\tST:%lld\tED:%lld\tMaxOutEdge:%lld\tCertained:%s\n", 
        id, cost, layer, in_degree, out_degree, topo_order, pe_id, round, retiming, 
        start_time, end_time, max_out_edge, (certain ? "Certained" : "UnCertained"));
  }
};

struct Edge {
  int from;
  int to;
  int round;
  long long memory;

  Edge() { }

  Edge(int a, int b, int c, long long d) {
    from = a;
    to = b;
    round = c;
    memory = d;
  }

  void Show() {
    printf("Round:%d\tFrom:%d\tTo:%d\tMemory:%lld\tCacheTimeCost:%lld\tDRAMTimeCost:%lld\n", 
          round, from, to, memory, Ceil(memory, CACHESPEED), Ceil(memory, DRAMSPEED));
  }
};

// PEId:      small -> big
// StartTime: small -> big
struct PEInterval {
  int pe_id;
  long long start_time;
  long long end_time;
  bool valid;

  PEInterval() {
    pe_id = -1;
    start_time = end_time = -1;
    valid = false;
  }

  PEInterval(int a, long long b, long long c) {
    pe_id = a;
    start_time = b;
    end_time = c;
    valid = true;
  }

  void SetTime(long long a, long long b) {
    assert(start_time <= end_time);
    start_time = a;
    end_time = b;
  }

  void Show() {
    printf("PE:%d [%lld, %lld]\n", pe_id, start_time, end_time);
  }

  friend bool operator< (PEInterval a, PEInterval b) {
    if (a.pe_id != b.pe_id)
      return a.pe_id < b.pe_id;
    return a.start_time < b.start_time;
  }
};

// EndTime:   small -> big
// PEId:      small -> big
struct TimeInterval : PEInterval {
  int count;

  TimeInterval(int a, long long b, long long c) : PEInterval(a, b, c) {
    count = 0;
  }

  friend bool operator< (TimeInterval a, TimeInterval b) {
    if (a.end_time != b.end_time)
      return a.end_time > b.end_time;
    return a.pe_id > b.pe_id;
  }
};

vector<Edge> edge_list[MAXN];
vector<Edge> re_edge_list[MAXN];
int pe_edges[MAXPE][MAXPE];
Node node_list[MAXN];
int degree[MAXN];
long long dp_array[MAXN][MAXSIZE + 1];
bool rechecked[MAXN][MINR];
bool visited[MAXN][MINR];
int node_count[MAXN][MINR];

int total_node, total_edge;
int total_pe, total_rounds, round_limit;
long long total_cost;


// first:     big -> small
// second:    small -> big
bool CmpByFirst(TwoInt a, TwoInt b) {
  if (a.first != b.first)
    return a.first > b.first;
  return a.second > b.second;
}

// Layer:     small -> big
// TopoOrder: small -> big
bool CmpByLayer(Node a, Node b) {
  if (a.layer != b.layer)
    return a.layer < b.layer;
  return a.topo_order < b.topo_order;
}

// TopoOrder: small -> big
// Id:        small -> big
// Round:     small -> big
bool CmpByTopoOrder(Node a, Node b) {
  if (a.topo_order != b.topo_order)
    return a.topo_order < b.topo_order;
  else if (a.id != b.id)
    return a.id < b.id;
  return a.round < b.round;
}

// PENumb:    small -> big
bool CmpByPENumb(Node a, Node b) {
  return a.pe_numb < b.pe_numb;
}

// Id:        small -> big
// Round:     small -> big
bool CmpById(Node a, Node b) {
  if (a.id != b.id)
    return a.id < b.id;
  return a.round < b.round;
}

// Cost:      big -> small
// Id:        small -> big
bool CmpByCost(Node a, Node b) {
  if (a.cost != b.cost)
    return a.cost > b.cost;
  return a.id < b.id;
}

// PE:        small -> big
// StartTime: small -> big
// EndTime:   small -> big
bool CmpByPE(Node a, Node b) {
  if (a.pe_id != b.pe_id)
    return a.pe_id < b.pe_id;
  else if (a.start_time != b.start_time)
    return a.start_time < b.start_time;
  return a.end_time < b.end_time;
}

// StartTime: small -> big
// EndTime:   small -> big
bool CmpByTime(Node a, Node b) {
  if (a.start_time != b.start_time)
    return a.start_time < b.start_time;
  return a.end_time < b.end_time;
}

// Cost:      big -> small
// FromId:    small -> big
bool CmpPreEdgeByFromCost(Edge a, Edge b) {
  if (node_list[a.from].topo_order != node_list[b.from].topo_order)
    return node_list[a.from].topo_order > node_list[b.from].topo_order;
  else if (node_list[a.from].cost != node_list[b.from].cost)
    return node_list[a.from].cost > node_list[b.from].cost;
  return a.from < b.from;
}

bool CmpEdge(Edge a, Edge b) {
  if (a.from != b.from)
    return a.from < b.from;
  return a.to < b.to;
}

// EndTime:   small -> big
// PEId:      small -> big
struct NodeComparationByEndTime {
  bool operator() (const Node &a, const Node &b) const {
    if (a.end_time != b.end_time)
      return a.end_time > b.end_time;
    return a.pe_id > b.pe_id;    
  }
};

// // TopoOrder: big -> small
// // cost:      big -> small
// // id:        small -> big
struct NodeComparationByCost {
  bool operator() (const Node &a, const Node &b) const {
    if (a.topo_order != b.topo_order)
      return a.topo_order < b.topo_order;
    else if (a.cost != b.cost)
      return a.cost < b.cost;
    return a.id > b.id;
  }
};

struct NodeGenerator {
  int total_node;
  int pe_number;
  long long period_time;
  int period_round;
  long long prologue;
  int retiming;
  long long run_on_cache_n;
  long long run_on_dram_n;
  long long run_on_cache;
  long long run_on_dram;
  double cache_ratio;
  double period_ratio;
  vector<Node> node_arr;

  NodeGenerator() {
    total_node = 0;
    pe_number = 0;
    period_time = 0;
    period_round = 0;
    prologue = 0;
    retiming = 0;
    run_on_cache = run_on_dram = run_on_cache_n = run_on_dram_n = 0;
    period_ratio = cache_ratio = 0;
    node_arr.clear();
  }

  NodeGenerator(int a, int b) {
    total_node = a;
    pe_number = b;
    period_time = 0;
    period_round = 0;
    prologue = 0;
    retiming = 0;
    run_on_cache = run_on_dram = run_on_cache_n = run_on_dram_n = 0;
    period_ratio = cache_ratio = 0;
    node_arr.clear();
  }

  void AddOneRoundNodes(Node node_list[MAXN]) {
    period_round = period_round + 1;
    for (int i = 1; i <= total_node; ++ i) {
      Node node = node_list[i];
      node.round = period_round;
      node_arr.push_back(node);
    }
    sort(node_arr.begin(), node_arr.end(), CmpById);
  }

  void Reset() {
    for (int i = 0; i < node_arr.size(); ++ i) {
      node_arr[i].SetTime(0, 0);
      node_arr[i].pe_id = -1;
      node_arr[i].retiming = 0;
      node_arr[i].certain = false;
    }
  }

  double GenerateBasedRound(int round, Node node_list[MAXN]) {
    priority_queue<Node, vector<Node>, NodeComparationByEndTime> q;
    node_arr.clear();
    for (int i = 1; i <= pe_number; ++ i) {
      Node n = Node(0, 0);
      n.pe_id = i;
      n.SetTime(0, 0);
      q.push(n);
    }
    sort(node_list + 1, node_list + total_node + 1, CmpByCost);
    for (int i = 1; i <= total_node; ++ i) {
      for (int j = 1; j <= round; ++ j) {
        Node n_top = q.top();
        q.pop();

        Node n = Node();
        n.Copy(node_list[i]);
        n.round = j;
        n.pe_id = n_top.pe_id;
        n.SetTime(n_top.end_time, n_top.end_time + n.cost);
        q.push(n);
        period_time = max(period_time, n_top.end_time + n.cost);
        node_arr.push_back(n);
      }
    }
    sort(node_list + 1, node_list + total_node + 1, CmpById);
    // calculate the use ratio of cpu
    assert(period_time != 0);
    double total = period_time * pe_number;
    double sum = 0;
    for (int i = 1; i <= total_node; ++ i)
      sum = sum + node_list[i].cost;
    sum = sum * round;
    double ratio = sum / total;
    return ratio;
  }

  void GeneratePeriodSchedule(int round_limit, Node node_list[MAXN]) {
    for (int i = 1; i <= round_limit; ++ i) {
      double current_ratio = GenerateBasedRound(i, node_list);
      if (current_ratio >= LIMITEDRATIO) {
        period_round = i;
        break;
      }
      else if (current_ratio > period_ratio) {
        period_round = i;
        period_ratio = current_ratio;
      }
    }
    period_ratio = GenerateBasedRound(period_round, node_list);
    sort(node_arr.begin(), node_arr.end(), CmpById);
  }

  void SetNode(Node node) {
    bool found = false;
    TwoInt interval = BinarySearch(node.id);
    for (int i = interval.first; i <= interval.second; ++ i) {
      if (node_arr[i].round == node.round) {
        if (node.start_time < 0 || node.end_time > period_time) {
          printf("### Bad Time ###\n");
          node.Show();
        }
        assert(node.start_time >= 0 && node.end_time <= period_time);
        node_arr[i].pe_id = node.pe_id;
        node_arr[i].SetTime(node.start_time, node.end_time);
        node_arr[i].retiming = node.retiming;
        node_arr[i].certain = node.certain;
        found = true;
        break;
      }
    }
    if (!found) {
      printf("### Cannot found target node ###\n");
      node.Show();
    }
    assert(found == true);
  }

  Node GetNode(int node_id, int round) {
    TwoInt interval = BinarySearch(node_id);
    for (int i = interval.first; i <= interval.second; ++ i)
      if (node_arr[i].round == round)
        return node_arr[i];
    printf("### Not Found Node:id:%d\tRound:%d ###\n", node_id, round);
    assert(false);
    return Node();
  }

  TwoInt BinarySearch(int node_id) {
    int left = 0;
    int right = (int)(node_arr.size()) - 1;
    if (node_arr[left].id == node_id) {
      right = left + period_round;
    }
    else {
      while (right - left > 1) {
        int middle = (left + right) >> 1;
        if (node_arr[middle].id < node_id)
          left = middle;
        else
          right = middle;
      }
      left = right;
      right = left + period_round;
    }
    // 返回闭区间[, ]
    return make_pair(left, right);
  }

  vector<Node> GetSamePEOtherNodes(int node_id, int round, PEInterval pe_int) {
    vector<Node> choosed;
    for (int i = 0; i < node_arr.size(); ++ i) {
      if (node_arr[i].pe_id != pe_int.pe_id)
        continue;
      if (node_arr[i].id == node_id && node_arr[i].round == round)
        continue;
      if (!node_arr[i].certain && 
          node_arr[i].start_time >= pe_int.start_time && 
          node_arr[i].end_time <= pe_int.end_time)
        choosed.push_back(node_arr[i]);
    }
    return choosed;
  }

  void CalcPrologue() {
    for (int i = 0; i < node_arr.size(); ++ i)
      retiming = min(retiming, node_arr[i].retiming);
    retiming = -retiming;
    prologue = 1LL * retiming * period_time;
  }

  void Show() {
    sort(node_arr.begin(), node_arr.end(), CmpByPE);
    int last_pe_id = -1;
    int last_end_time = 0;
    printf("Arrangement");
    for (int i = 0; i < node_arr.size(); ++ i) {
      if (node_arr[i].pe_id != last_pe_id) {
        if (last_end_time != period_time && last_pe_id != -1) {
          for (int j = last_end_time; j < period_time; ++ j)
            printf("--");
        }
        printf("\nPE%2d:", node_arr[i].pe_id);
        last_pe_id = node_arr[i].pe_id;
        last_end_time = 0;
      }
      if (last_end_time < node_arr[i].start_time)
        for (int j = last_end_time; j < node_arr[i].start_time; ++ j)
          printf("--");
      char c =  'A' + node_arr[i].id - 1;
      for (int j = 0; j < node_arr[i].cost; ++ j)
        printf("%c%d", c, node_arr[i].round);
      last_end_time = node_arr[i].end_time;
    }
    if (last_end_time != period_time && last_pe_id != -1) {
      for (int j = last_end_time; j < period_time; ++ j)
        printf("--");
    }
    printf("\n\n");
    sort(node_arr.begin(), node_arr.end(), CmpById);
  }

  void ShowEach(bool only_uncertain) {
    for (int i = 0; i < node_arr.size(); ++ i) {
      if ((only_uncertain && !node_arr[i].certain) || !only_uncertain)
        node_arr[i].Show();
    }
  }
};

int GetTopology() {
  int vis[MAXR];
  memset(degree, 0, sizeof(degree));
  memset(vis, 0, sizeof(vis));
  for (int i = 1; i <= total_node; ++ i) {
    for (int j = 0; j < edge_list[i].size(); ++ j) {
      Edge e = edge_list[i][j];
      degree[e.to] = degree[e.to] + 1;
      node_list[e.from].max_out_edge = max(node_list[e.from].max_out_edge, e.memory);
    }
  }

  int count = 0, order = 0, concurrency = 0;
  queue<Node> q;
  for (int i = 1; i <= total_node; ++ i)
    if (degree[i] == 0)
      q.push(node_list[i]);
  concurrency = count = q.size();
  vis[count] = vis[count] + 1;
  while (!q.empty()) {
    Node f = q.front();
    q.pop();
    node_list[f.id].topo_order = order;
    count = count - 1;

    for (int i = 0; i < edge_list[f.id].size(); ++ i) {
      Edge e = edge_list[f.id][i];
      degree[e.to] = degree[e.to] - 1;
      if (degree[e.to] == 0) {
        q.push(node_list[e.to]);
      }
    }

    if (count == 0) {
      count = q.size();
      concurrency = max(concurrency, count);
      order = order + 1;
      vis[count] = vis[count] + 1;
    }
  }

  if (concurrency > total_pe) {
    int max_count = vis[0];
    concurrency = 0;
    for (int i = 1; i < order; ++ i) {
      if (max_count < vis[i]) {
        max_count = vis[i];
        concurrency = i;
      }
    }
  }

  // sort(node_list + 1, node_list + total_node + 1, CmpByLayer);
  // count = 0, order = 0;
  // concurrency = 0;
  // for (int i = 1; i <= total_node; ++ i) {
  //   if (node_list[i].layer != order) {
  //     concurrency = max(concurrency, count);
  //     count = 0;
  //     order = node_list[i].layer;
  //   }
  //   count = count + 1;
  // }
  // concurrency = max(concurrency, count);
  return concurrency;
}

set<int> Dynamic(vector<TwoInt> paddings, int bin_size) {
  set<int> choosed;
  memset(dp_array, 0, sizeof(dp_array));
  for (int i = 1; i <= paddings.size(); ++ i) {
    int padding = paddings[i - 1].first;
    for (int j = bin_size; j >= 0; -- j) {
      if (j >= padding && dp_array[i - 1][j - padding] + padding >= dp_array[i][j])
        dp_array[i][j] = max(dp_array[i - 1][j], dp_array[i - 1][j - padding] + padding);
      else
        dp_array[i][j] = dp_array[i - 1][j];
    }
  }

  int k = bin_size;
  for (int i = paddings.size(); i > 0; -- i) {
    int padding = paddings[i - 1].first;
    if (k >= padding && dp_array[i][k] == dp_array[i - 1][k - padding] + padding) {
      k = k - padding;
      choosed.insert(paddings[i - 1].second);
    }
  }
  return choosed; 
}

set<int> Greedy(vector<TwoInt> paddings, int bin_size) {
  set<int> choosed;
  sort(paddings.begin(), paddings.end(), CmpByFirst);
  for (int i = 0; i < paddings.size(); ++ i) {
    TwoInt padding = paddings[i];
    if (bin_size >= padding.first) {
      choosed.insert(padding.second);
      bin_size = bin_size - padding.first;
    }
  }
  return choosed;  
}

vector<bool> ArrangeInFixedSize(vector<int> paddings, long long bin_size, string algo) {
  vector<bool> result;
  set<int> choosed;
  long long sum = 0;

  for (int i = 0; i < paddings.size(); ++ i) {
    result.push_back(false);
    sum = sum + paddings[i];
  }

  if (sum <= bin_size) {
    // 全部够放
    for (int i = 0; i < paddings.size(); ++ i)
      choosed.insert(i);
  }
  else {
    // 部分够放
    // 首先去除超出总大小的单件物品
    sum = 0;
    vector<TwoInt> rest_paddings;
    for (int i = 0; i < paddings.size(); i++) {
      if (paddings[i] <= bin_size) {
        sum = sum + paddings[i];
        rest_paddings.push_back(make_pair(paddings[i], i));
      }
    }


    if (sum != 0) {
      if (algo == "Greedy") {
        // 使用贪心算法
        choosed = Greedy(rest_paddings, bin_size);
      }
      else if (algo == "Dynamic") {
        if (rest_paddings.size() >= MAXN || bin_size > MAXSIZE) {
          // 超出动态规划使用前提条件
          choosed = Greedy(rest_paddings, bin_size);
        }
        else {
          // 使用动态规划算法
          choosed = Dynamic(rest_paddings, bin_size);
        }
      }
      else {
        // 使用先到先得算法
        long long cur_sum = 0;
        for (int i = 0; i < rest_paddings.size(); ++ i) {
          if (cur_sum + rest_paddings[i].first > bin_size)
            continue;
          cur_sum = cur_sum + rest_paddings[i].first;
          choosed.insert(rest_paddings[i].second);
        }
      }
    }
  }
  for (set<int>::iterator it = choosed.begin(); it != choosed.end(); ++ it)
    result[*it] = true;
  return result;
}

bool UpdatePrecursorRetiming(Node &from_node, Node to_node, long long period_time, long long cost) {
  if (from_node.end_time + from_node.retiming * period_time + cost > 
    to_node.start_time + to_node.retiming * period_time) {
    from_node.retiming = Floor(to_node.start_time + to_node.retiming * period_time - cost - from_node.end_time, period_time);
    return true;
  }
  return false;
}

FinalResult CalcFinalResult(vector<NodeGenerator> ng_list) {
  FinalResult final_result;
  if (ng_list.size() == 1) {
    assert(total_pe % ng_list[0].pe_number == 0);
    int launch_number = total_pe / ng_list[0].pe_number;
    int period_number = Floor(Floor(total_rounds, launch_number), ng_list[0].period_round);
    final_result.total_time = ng_list[0].prologue + 1LL * period_number * ng_list[0].period_time;
    final_result.period_time = ng_list[0].period_time;
    final_result.prologue = ng_list[0].prologue;
    final_result.retiming = ng_list[0].retiming;
    final_result.run_on_cache_n = ng_list[0].run_on_cache_n * period_number * launch_number;
    final_result.run_on_dram_n = ng_list[0].run_on_dram_n * period_number * launch_number;
    final_result.run_on_cache = ng_list[0].run_on_cache * period_number * launch_number;
    final_result.run_on_dram = ng_list[0].run_on_dram * period_number * launch_number;
    final_result.cache_ratio = ng_list[0].cache_ratio;
    final_result.cpu_ratio = 1.0 * (total_rounds * total_cost) / (final_result.total_time * total_pe);
    final_result.period_ratio = ng_list[0].period_ratio;
  }
  else {
    int launch_number = total_pe / ng_list[0].pe_number;
    for (int round0 = 0; round0 <= total_rounds; ++ round0) {
      int round1 = total_rounds - round0;
      int period_number0 = Floor(Floor(round0, launch_number), ng_list[0].period_round);
      int period_number1 = Floor(round1, ng_list[1].period_round);
      long long total_time0 = ng_list[0].prologue + 1LL * period_number0 * ng_list[0].period_time;
      long long total_time1 = ng_list[1].prologue + 1LL * period_number1 * ng_list[1].period_time;
      long long total_time = max(total_time0, total_time1);
      if (final_result.total_time == -1 || total_time < final_result.total_time) {
        final_result.total_time = total_time;
        final_result.period_time = (ng_list[0].period_time + ng_list[1].period_time) / 2.0;
        final_result.run_on_cache_n = ng_list[0].run_on_cache_n * period_number0 * launch_number + ng_list[1].run_on_cache_n * period_number1;
        final_result.run_on_dram_n = ng_list[0].run_on_dram_n * period_number0 * launch_number + ng_list[1].run_on_dram_n * period_number1;
        final_result.run_on_cache = ng_list[0].run_on_cache * period_number0 * launch_number + ng_list[1].run_on_cache * period_number1;
        final_result.run_on_dram = ng_list[0].run_on_dram * period_number0 * launch_number + ng_list[1].run_on_dram * period_number1;
      }
    }
    final_result.prologue = ng_list[0].prologue;
    final_result.retiming = ng_list[0].retiming;
    final_result.cpu_ratio = 1.0 * (total_rounds * total_cost) / (final_result.total_time * total_pe);
    final_result.period_ratio = (ng_list[0].period_ratio + ng_list[1].period_ratio) / 2;
  }
  return final_result;
}
