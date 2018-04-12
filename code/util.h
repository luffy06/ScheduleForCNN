#define THEORY 2
#define MAXM 70000
#define MAXN 1500
#define MAXSIZE 50000
#define MAXPE 300
#define MINR 505
#define MAXR 6005
#define LIMITEDRATIO 0.85
#define ALPHA 0.8
const int INF = 0x3f3f3f3f;
const long long DRAMSPEED = 10000;
const long long CACHESPEED = 100000;
const long long CACHESIZE = 20480;

typedef pair<int, int> TwoInt;

void TestInput() {
  int t;
  scanf("%d", &t);
}

long long Ceil(long long a, long long b) {
  if (a % b == 0)
    return a / b;
  return a / b + 1;
}

long long Floor(long long a, long long b) {
  if (a % b == 0)
    return a / b;
  return a / b - 1;
}

struct FinalResult {
  long long TotalTime;
  long long Prelogue;
  int Retiming;
  int RunOnCache;
  int RunOnDRAM;
  double MAXRatio;
  double CPURatio;

  FinalResult() {
    TotalTime = -1;
    Prelogue = 0;
    Retiming = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    MAXRatio = 0;
    CPURatio = 0;
  }

  FinalResult(int a, int b, int c, int d, int e, double f, double g) {
    TotalTime = a;
    Prelogue = b;
    Retiming = c;
    RunOnCache = d;
    RunOnDRAM = e;
    MAXRatio = f;
    CPURatio = g;
  }

  void Show() {
    printf("\nTotalTime:%lld\nKernel:%lld\nPrelogue:%lld\nRetiming:%d\nRunOnCache:%d\nRunOnDRAM:%d\nMAXRatio:%.6f\nCPURatio:%.6f\n", 
            TotalTime, TotalTime - 2 * Prelogue, Prelogue, Retiming, RunOnCache, RunOnDRAM, MAXRatio, CPURatio);
  }
};

struct CacheBlock {
  int Id;
  TwoInt NodeIds;
  TwoInt Rounds;
  long long Memory;
  long long StartTime;
  long long EndTime;
  bool Exist;

  CacheBlock(int From, int FromRound, int To, int ToRound, long long Memo, long long ST, long long ED) {
    NodeIds = make_pair(From, To);
    Rounds = make_pair(FromRound, ToRound);
    Memory = Memo;
    StartTime = ST;
    EndTime = ED;
    if (ST >= ED) {
      printf("%lld %lld\n", ST, ED);
    }
    Id = -1;
    Exist = true;
    assert(ST < ED);
  }

  bool Equal(CacheBlock CB) {
    if (NodeIds.first == CB.NodeIds.first && NodeIds.second == CB.NodeIds.second
      && Rounds.first == CB.Rounds.first && Rounds.second == CB.Rounds.second
      && Memory == CB.Memory && StartTime == CB.StartTime && EndTime == CB.EndTime)
      return true;
    return false;
  }

  void Show() {
    printf("Id:%d\tFrom:%d-%d\tTo:%d-%d\tST:%lld\tED:%lld\tMemory:%lld\n", Id, NodeIds.first, 
            Rounds.first, NodeIds.second, Rounds.second, StartTime, EndTime, Memory);
  }

  friend bool operator < (CacheBlock a, CacheBlock b) {
    if (a.StartTime != b.StartTime)
      return a.StartTime < b.StartTime;
    return a.EndTime < b.EndTime;
  }
};

struct CacheManager {
  int PEId;
  vector<CacheBlock> Cache;
  int Count;
  set<long long> TimeTrace;
  
  CacheManager() {
    PEId = -1;
    Count = 0;
    Cache.clear();
    TimeTrace.clear();
    TimeTrace.insert(0LL);
  }

  void SortCacheBlock() {
    sort(Cache.begin(), Cache.end());
    for (int i = 0; i < Cache.size(); ++ i)
      Cache[i].Id = i;
    // for (int i = 0; i < Cache.size(); ++ i)
    //   printf("%lld %lld\n", Cache[i].StartTime, Cache[i].EndTime);
  }

  void AddCacheBlock(CacheBlock Block) {
    Cache.push_back(Block);
    TimeTrace.insert(Block.StartTime);
    TimeTrace.insert(Block.EndTime);
  }

  void DeleteCacheBlock(CacheBlock Block) {
    // [L, R)
    int L = 0, R = Cache.size() - 1;
    if (Block.Id == Cache[R].Id) {
      Cache[R].Exist = false;
      return;
    }
    while (R - L > 1) {
      int M = (L + R) / 2;
      if (Cache[M].Id <= Block.Id) L = M;
      else R = M;
    }
    if (Cache[L].Id == Block.Id) {
      Cache[L].Exist = false;
      return ;
    }
    printf("Cannnot find Cache Block\n");
    Block.Show();
    for (int i = 0; i < Cache.size(); ++ i) {
      if (Block.Id == Cache[i].Id) {
        printf("Find %d\n", i);
      }
    }
    assert(1 == 0);
  }

  // OPTIMIZING
  int GetCacheBlockByTime(long long StartTime, long long EndTime, vector<CacheBlock> &Blocks, long long &MemorySum, int Index) {
    MemorySum = 0;
    for (int i = Index; i < Cache.size(); ++ i) {
      if (Cache[i].StartTime > StartTime) {
        Index = i;
        break;
      }
      if (Cache[i].Exist == false)
        continue;
      if (Cache[i].StartTime <= StartTime && Cache[i].EndTime >= EndTime) {
        MemorySum = MemorySum + Cache[i].Memory;
        Blocks.push_back(Cache[i]);
      }
    }
    return Index;
  }

  vector<long long> GetTimeTrace() {
    vector<long long> Res;
    for (set<long long>::iterator it = TimeTrace.begin(); it != TimeTrace.end(); ++ it)
      Res.push_back((*it));
    // printf("TimeTrace Size:%lu Min:%lld Max:%lld\n", Res.size(), Res[0], Res[Res.size() - 1]);
    return Res;
  }
};

struct Node {
  int Id;
  long long Cost;
  char Name[200];
  int Layer;
  int InDegree;
  int OutDegree;
  int TopoOrder;

  int PEId;
  int Round;
  int Retiming;

  long long StartTime;
  long long EndTime;

  long long MaxOutEdge;
  bool Certained;
  int PENumb;

  Node() {
    Id = -1;
    Cost = 0;
    Layer = -1;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
    PEId = -1;
    Round = -1;
    Retiming = 0;
    StartTime = EndTime = -1;
    MaxOutEdge = 0;
    Certained = false;
    PENumb = -1;
  }

  Node(int a, long long b) {
    Id = a;
    Cost = b;
    Layer = -1;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
    PEId = -1;
    Round = -1;
    Retiming = 0;
    StartTime = EndTime = -1;
    MaxOutEdge = 0;
    Certained = false;
    PENumb = -1;
  }

  void SetTime(long long ST, long long ED) {
    StartTime = ST;
    EndTime = ED;
    assert(StartTime <= EndTime);
  }

  void Copy(Node t) {
    Id = t.Id;
    Cost = t.Cost;
    strcpy(Name, t.Name);
    Layer = t.Layer;
    InDegree = t.InDegree;
    OutDegree = t.OutDegree;
    TopoOrder = t.TopoOrder;
    PEId = t.PEId;
    Round = t.Round;
    Retiming = t.Retiming;
    StartTime = t.StartTime;
    EndTime = t.EndTime;
    MaxOutEdge = t.MaxOutEdge;
    Certained = t.Certained;
  }

  void Show() {
    printf("ID:%d\tCost:%lld\tLayer:%d\tInDe:%d\tOutDe:%d\tTopo:%d\tPEId:%d\tRound:%d\tRetiming:%d\tST:%lld\tED:%lld\tMaxOutEdge:%lld\tCertained:%s\n", 
        Id, Cost, Layer, InDegree, OutDegree, TopoOrder, PEId, Round, Retiming, 
        StartTime, EndTime, MaxOutEdge, (Certained ? "Certained" : "UnCertained"));
  }
};

struct Edge {
  int From;
  int To;
  long long Memory;

  Edge() { }

  Edge(int a, int b, long long c) {
    From = a;
    To = b;
    Memory = c;
  }

  void Show() {
    printf("From:%d\tTo:%d\tMemory:%lld\tCacheTimeCost:%lld\tDRAMTimeCost:%lld\n", 
          From, To, Memory, Ceil(Memory, CACHESPEED), Ceil(Memory, DRAMSPEED));
  }
};

// PEId:      small -> big
// StartTime: small -> big
struct PEInterval {
  int PEId;
  long long StartTime;
  long long EndTime;

  PEInterval(int a, long long b, long long c) {
    PEId = a;
    StartTime = b;
    EndTime = c;
  }

  void SetTime(long long a, long long b) {
    assert(StartTime <= EndTime);
    StartTime = a;
    EndTime = b;
  }

  friend bool operator < (PEInterval a, PEInterval b) {
    if (a.PEId != b.PEId)
      return a.PEId < b.PEId;
    return a.StartTime < b.StartTime;
  }
};

// EndTime:   small -> big
// PEId:      small -> big
struct TimeInterval : PEInterval {
  int Count;

  TimeInterval(int a, long long b, long long c) : PEInterval(a, b, c) {
    Count = 0;
  }

  friend bool operator< (TimeInterval a, TimeInterval b) {
    if (a.EndTime != b.EndTime)
      return a.EndTime < b.EndTime;
    return a.PEId < b.PEId;
  }
};


vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
Node NodeList[MAXN];
int Degree[MAXN];
int DP[MAXN][MAXSIZE + 1];

vector<CacheManager> Caches;
vector<CacheBlock> DRAMBlocks;

int TotalNode;
int TotalPE, PeriodTimes, UpRound;


// first:     big -> small
// second:    small -> big
bool CmpByFirst(TwoInt a, TwoInt b) {
  if (a.first != b.first)
    return a.first > b.first;
  return a.second < b.second;
}

// Layer:     small -> big
// TopoOrder: small -> big
bool CmpByLayer(Node a, Node b) {
  if (a.Layer != b.Layer)
    return a.Layer < b.Layer;
  return a.TopoOrder < b.TopoOrder;
}

// TopoOrder: small -> big
// Id:        small -> big
// Round:     small -> big
bool CmpByTopoOrder(Node a, Node b) {
  if (a.TopoOrder != b.TopoOrder)
    return a.TopoOrder < b.TopoOrder;
  else if (a.Id != b.Id)
    return a.Id < b.Id;
  return a.Round < b.Round;
}

// PENumb:    small -> big
bool CmpByPENumb(Node a, Node b) {
  return a.PENumb < b.PENumb;
}

// Id:        small -> big
// Round:     small -> big
bool CmpById(Node a, Node b) {
  if (a.Id != b.Id)
    return a.Id < b.Id;
  return a.Round < b.Round;
}

// Cost:      big -> small
// Id:        small -> big
bool CmpByCost(Node a, Node b) {
  if (a.Cost != b.Cost)
    return a.Cost > b.Cost;
  return a.Id < b.Id;
}

// PE:        small -> big
// StartTime: small -> big
// EndTime:   small -> big
bool CmpByPE(Node a, Node b) {
  if (a.PEId != b.PEId)
    return a.PEId < b.PEId;
  else if (a.StartTime != b.StartTime)
    return a.StartTime < b.StartTime;
  return a.EndTime < b.EndTime;
}

// StartTime: small -> big
// EndTime:   small -> big
bool CmpByTime(Node a, Node b) {
  if (a.StartTime != b.StartTime)
    return a.StartTime < b.StartTime;
  return a.EndTime < b.EndTime;
}

// Cost:      big -> small
// FromId:    small -> big
bool CmpEdgeByFromCost(Edge a, Edge b) {
  if (NodeList[a.From].Cost != NodeList[b.From].Cost)
    return NodeList[a.From].Cost > NodeList[b.From].Cost;
  return a.From < b.From;
}

// EndTime:   small -> big
// PEId:      small -> big
struct NodeComparationByEndTime {
  bool operator() (const Node &a, const Node &b) const {
    if (a.EndTime != b.EndTime)
      return a.EndTime > b.EndTime;
    return a.PEId > b.PEId;    
  }
};

int GetTopology() {
  memset(Degree, 0, sizeof(Degree));
  vector<TwoInt> TopoCount;
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
      NodeList[e.From].MaxOutEdge = max(NodeList[e.From].MaxOutEdge, e.Memory);
    }
  }

  int Count = 0, Order = 0;
  queue<Node> q;
  for (int i = 1; i <= TotalNode; ++ i)
    if (Degree[i] == 0)
      q.push(NodeList[i]);
  Count = q.size();

  while (!q.empty()) {
    Node f = q.front();
    q.pop();
    NodeList[f.Id].TopoOrder = Order;
    Count = Count - 1;

    for (int i = 0; i < EdgeList[f.Id].size(); ++ i) {
      Edge e = EdgeList[f.Id][i];
      Degree[e.To] = Degree[e.To] - 1;
      if (Degree[e.To] == 0) 
        q.push(NodeList[e.To]);
    }

    if (Count == 0) {
      Count = q.size();
      Order = Order + 1;
    }
  }

  sort(NodeList + 1, NodeList + TotalNode + 1, CmpByLayer);
  Count = 0, Order = 0;
  int NeedPE = 0;
  for (int i = 1; i <= TotalNode; ++ i) {
    if (NodeList[i].Layer != Order) {
      NeedPE = max(NeedPE, Count);
      Count = 0;
      Order = NodeList[i].Layer;
    }
    Count = Count + 1;
  }
  NeedPE = max(NeedPE, Count);
  return NeedPE;
}

set<int> Dynamic(vector<TwoInt> Goods, int BinSize) {
  set<int> ArrangedGoods;
  memset(DP, 0, sizeof(DP));
  for (int i = 1; i <= Goods.size(); ++ i) {
    int S = Goods[i - 1].first;
    for (int j = BinSize; j >= 0; -- j) {
      if (j >= S && DP[i - 1][j - S] + S > DP[i][j])
        DP[i][j] = max(DP[i - 1][j], DP[i - 1][j - S] + S);
      else
        DP[i][j] = DP[i - 1][j];
    }
  }

  int k = BinSize;
  for (int i = Goods.size(); i > 0; -- i) {
    int S = Goods[i - 1].first;
    if (k >= S && DP[i][k] == DP[i - 1][k - S] + S) {
      k = k - S;
      ArrangedGoods.insert(Goods[i - 1].second);
    }
  }
  return ArrangedGoods;  
}

set<int> Greedy(vector<TwoInt> Goods, int BinSize) {
  set<int> ArrangedGoods;
  sort(Goods.begin(), Goods.end(), CmpByFirst);
  for (int i = 0; i < Goods.size(); ++ i) {
    TwoInt good = Goods[i];
    if (BinSize >= good.second) {
      ArrangedGoods.insert(good.second);
      BinSize = BinSize - good.second;
    }
  }
  return ArrangedGoods;  
}

set<int> ArrangeInFixedSize(vector<int> Goods, int BinSize) {
  set<int> ArrangedGoods;
  long long Sum = 0;
  for (int i = 0; i < Goods.size(); ++ i)
    Sum = Sum + Goods[i];
  if (Sum <= BinSize) {
    for (int i = 0; i < Goods.size(); ++ i)
      ArrangedGoods.insert(i);
    return ArrangedGoods;
  }

  Sum = 0;
  vector<TwoInt> RestGoods;
  for (int i = 0; i < Goods.size(); i++) {
    if (Goods[i] <= BinSize) {
      Sum = Sum + Goods[i];
      RestGoods.push_back(make_pair(Goods[i], i));
    }
  }

  if (Sum == 0)
    return ArrangedGoods;

  if (RestGoods.size() >= MAXN || BinSize > MAXSIZE) {
    // printf("Greedy\n");
    return Greedy(RestGoods, BinSize);
  }
  else {
    // printf("Dynamic\n");
    return Dynamic(RestGoods, BinSize);
  }
}
