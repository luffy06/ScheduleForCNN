
Node NodeList[MAXN];

bool CmpByCost(Node a, Node b) {
  if (a.Cost != b.Cost)
    return a.Cost > b.Cost;
  return a.Id < b.Id;
}

bool CmpById(Node a, Node b) {
  if (a.Id != b.Id)
    return a.Id < b.Id;
  return a.Round < b.Round;
}

bool CmpByPE(Node a, Node b) {
  if (a.PEId != b.PEId)
    return a.PEId < b.PEId;
  else if (a.StartTime != b.StartTime)
    return a.StartTime < b.StartTime;
  else if (a.EndTime != b.EndTime)
    return a.EndTime < b.EndTime;
  else if (a.TopoOrder != b.TopoOrder)
    return a.TopoOrder < b.TopoOrder;
  else if (a.Round != b.Round)
    return a.Round < b.Round;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
}

bool CmpByTopoOrder(Node a, Node b) {
  if (a.TopoOrder != b.TopoOrder)
    return a.TopoOrder < b.TopoOrder;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
}

bool CmpEdgeByFromCost(Edge a, Edge b) {
  if (NodeList[a.From].Cost != NodeList[b.From].Cost)
    return NodeList[a.From].Cost > NodeList[b.From].Cost;
  return a.From < b.From;
}

struct NodeComparationByEndTime {
  bool operator() (const Node &a, const Node &b) const {
    if (a.EndTime != b.EndTime)
      return a.EndTime > b.EndTime;
    return a.PEId > b.PEId;    
  }
};

struct NodeComparationByCost {
  bool operator() (const Node &a, const Node &b) const {
    if (a.TopoOrder != b.TopoOrder)
      return a.TopoOrder < b.TopoOrder;
    else if (a.Cost != b.Cost)
      return a.Cost < b.Cost;
    return a.Id > b.Id;    
  }
};

struct Iteration {
  int PENumb;
  long long Cost;
  int Round;
  int RunOnCache;
  int RunOnDRAM;
  int PEEndtime[MAXPE];
  
  Iteration(int a) {
    PENumb = a;
    Cost = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    Round = 1;
    memset(PEEndtime, 0, sizeof(PEEndtime));
  }
};

typedef pair<Edge, bool> EdgeBool;

vector<Iteration> IterList;
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;

vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
vector<PEInterval> PEIntervals[MAXPE];

int DP[MAXN][MAXSIZE];

Node NodeTime[MAXPE][MAXN];

vector<CacheManager> Caches;
vector<CacheBlock> DRAMBlocks;

int GetTopology() {
  int Count = 0, Order = 0;
  int NeedPE = 0;
  int MinCon = INF, MaxCon = -1;
  queue<Node> q;
  for (int i = 1; i <= TotalNode; ++ i) {
    if (Degree[i] == 0) {
      q.push(NodeList[i]);
    }
  }
  Count = NeedPE = q.size();
  MinCon = MaxCon = q.size();

  while (!q.empty()) {
    Node f = q.front();
    q.pop();
    NodeList[f.Id].TopoOrder = Order;
    Count = Count - 1;

    for (int i = 0; i < EdgeList[f.Id].size(); ++ i) {
      Edge e = EdgeList[f.Id][i];
      Degree[e.To] = Degree[e.To] - 1;
      if (Degree[e.To] == 0) {
        q.push(NodeList[e.To]);
      }
    }

    if (Count == 0) {
      NeedPE = max((int)q.size(), NeedPE);
      MaxCon = max((int)q.size(), MaxCon);
      if (!q.empty())
        MinCon = min((int)q.size(), MinCon);
      Count = q.size();
      Order = Order + 1;
    }
  }
  printf("MaxCon:%d\tMinCon:%d\tTopoOrder:%d\n", MaxCon, MinCon, Order);
  return NeedPE;
}

void Init(int TotalPE, int UpRound) {
  memset(Degree, 0, sizeof(Degree));
  Caches.clear();
  DRAMBlocks.clear();

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  int NeedPE = GetTopology();
  printf("Multi:%d\n", NeedPE);

  if (TotalPE >= NeedPE) {
    IterList.push_back(Iteration(NeedPE));
    if (TotalPE % NeedPE != 0)
      IterList.push_back(Iteration(TotalPE % NeedPE));
  }
  else {
    IterList.push_back(Iteration(TotalPE));
  }
}

vector<int> ArrangeInFixedSize(vector<int> Goods, int BinSize) {
  vector<int> ArrangedGoods, UnArrangedGoods;
  int Sum = 0;
  for (int i = 0; i < Goods.size(); ++ i)
    Sum = Sum + Goods[i];
  if (Sum <= BinSize) {
    for (int i = 0; i < Goods.size(); ++ i)
      ArrangedGoods.push_back(i);
    return ArrangedGoods;
  }

  memset(DP, 0, sizeof(DP));
  bool RE = false;
  if (BinSize > MAXSIZE) {
    RE = true;
    // printf("### Bad BinSize Of %d ###\n", BinSize);
    // printf("Good Size:%lu\n", Goods.size());
    BinSize = Sum - BinSize;
    // printf("BinSize:%d\tSum:%d\tMAXSIZE:%d\n", BinSize, Sum, MAXSIZE);
  }
  assert(BinSize <= MAXSIZE);

  for (int i = 1; i <= Goods.size(); ++ i) {
    int S = Goods[i - 1];
    for (int j = BinSize; j >= 0; -- j) {
      if (j >= S && DP[i - 1][j - S] + S > DP[i][j])
        DP[i][j] = max(DP[i - 1][j], DP[i - 1][j - S] + S);
      else
        DP[i][j] = DP[i - 1][j];
    }
  }

  int k = BinSize;
  for (int i = Goods.size(); i > 0; -- i) {
    int S = Goods[i - 1];
    if (k >= S && DP[i][k] == DP[i - 1][k - S] + S) {
      k = k - S;
      ArrangedGoods.push_back(i - 1);
    }
    else {
      UnArrangedGoods.push_back(i - 1);
    }
  }

  if (RE) {
    int Dis = BinSize - DP[Goods.size()][BinSize];
    if (Dis > 0) {
      int MinDis = INF;
      int Choose = -1;
      for (int i = 0; i < UnArrangedGoods.size(); ++ i) {
        if (Goods[UnArrangedGoods[i]] >= Dis && Goods[UnArrangedGoods[i]] - Dis < MinDis) {
          MinDis = Goods[UnArrangedGoods[i]] - Dis;
          Choose = i;
        }
      }
      UnArrangedGoods.erase(UnArrangedGoods.begin() + Choose);
    }
    sort(UnArrangedGoods.begin(), UnArrangedGoods.end());
    return UnArrangedGoods;
  }

  sort(ArrangedGoods.begin(), ArrangedGoods.end());
  return ArrangedGoods;  
}

void InitIteration(Iteration &iteration) {
  sort(NodeList + 1, NodeList + TotalNode + 1, CmpByTopoOrder);
  int NowOrder = -1;
  int Index = 1;
  queue<Node> WaitingQue;
  iteration.Round = iteration.PENumb;
  do {
    for (; Index <= TotalNode && NodeList[Index].TopoOrder == NowOrder; ++ Index) {
      for (int j = 1; j <= iteration.Round; ++ j) {
        NodeList[Index].Round = j;
        WaitingQue.push(NodeList[Index]);
      }
    }
    int PEIter = 1;
    while (!WaitingQue.empty()) {
      if (PEIter == iteration.PENumb + 1)
        PEIter = 1;
      Node node = WaitingQue.front();
      WaitingQue.pop();
      long long StartTime = iteration.PEEndtime[PEIter];
      
      vector<Edge> Edges = ReEdgeList[node.Id];
      vector<int> NodeSizes;
      vector<Edge> ReadyForCache;
      for (int i = 0; i < Edges.size(); ++ i) {
        Edge e = Edges[i];
        if (NodeTime[node.Round][e.From].EndTime + e.DRAMTimeCost > StartTime) {
          NodeSizes.push_back(e.Memory);
          ReadyForCache.push_back(e);
        }
        else {
          iteration.RunOnDRAM = iteration.RunOnDRAM + 1;
        }
      }
      vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, CACHESIZE);

      for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
        if (i > 0) assert(ArrangedSet[i] > ArrangedSet[i - 1]);
        Edge e = ReadyForCache[ArrangedSet[i]];
        StartTime = max(StartTime, e.CacheTimeCost + NodeTime[node.Round][e.From].EndTime);
        iteration.RunOnCache = iteration.RunOnCache + 1;
        ReadyForCache.erase(ReadyForCache.begin() + ArrangedSet[i]);
      }

      for (int i = 0; i < ReadyForCache.size(); ++ i) {
        Edge e = ReadyForCache[i];
        StartTime = max(StartTime, e.DRAMTimeCost + NodeTime[node.Round][e.From].EndTime);
        iteration.RunOnDRAM = iteration.RunOnDRAM + 1;
      }

      node.PEId = PEIter;
      node.SetTime(StartTime, StartTime + node.Cost);
      NodeTime[node.Round][node.Id].Copy(node);
      iteration.PEEndtime[PEIter] = max(iteration.PEEndtime[PEIter], (int)node.EndTime);
      iteration.Cost = max(iteration.Cost, 1LL * iteration.PEEndtime[PEIter]);
    }
    if (Index <= TotalNode)
      NowOrder = NodeList[Index].TopoOrder;
  } while (Index <= TotalNode);
}

FinalResult CalcBaseFinalResult(int TotalCost, int Launches, int PeriodTimes) {
  for (int i = 0; i < IterList.size(); ++ i) {
    printf("Init Iteration:%d\n", i + 1);
    for (int j = 1; j <= IterList[i])
    InitIteration(IterList[i]);
  }

  FinalResult FR = FinalResult();
  if (IterList.size() == 1) {
    int Each = Ceil(PeriodTimes, Launches);
    int X = Ceil(Each, IterList[0].Round);
    FR.TotalTime = X * IterList[0].Cost;
    FR.Prelogue = 0;
    FR.Retiming = 0;
    FR.RunOnCache = IterList[0].RunOnCache * X * Launches;
    FR.RunOnDRAM = IterList[0].RunOnDRAM * X * Launches;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
      int EachY = PeriodTimes - EachX;
      int X = Ceil(EachX, IterList[0].Round);
      int Y = Ceil(EachY, IterList[1].Round);
      long long TotalTimeX = IterList[0].Cost * X;
      long long TotalTimeY = IterList[1].Cost * Y;
      long long TotalTime = max(TotalTimeX, TotalTimeY);
      if (FR.TotalTime == -1 || TotalTime < FR.TotalTime) {
        FR.TotalTime = TotalTime;
        FR.RunOnCache = IterList[0].RunOnCache * X * Launches + IterList[1].RunOnCache * Y;
        FR.RunOnDRAM = IterList[0].RunOnDRAM * X * Launches + IterList[1].RunOnDRAM * Y;
      }
    }
    FR.Prelogue = 0;
    FR.Retiming = 0;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  return FR;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);
  // for (int i = 1; i <= TotalNode; ++ i) {
  //   vector<Edge> edges = EdgeList[i];
  //   for (int j = 0; j < edges.size(); ++ j) {
  //     Edge e = edges[j];
  //     printf("From:%d\tTo:%d\tCacheCost:%d\tDRAMCost:%d\n", e.From, e.To, e.CacheTimeCost, e.DRAMTimeCost);
  //   }
  // }

  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;
  int Launches = TotalPE / NgList[0].NeedPE;

  // FinalResult FR = CalcFinalResult(TotalCost, Launches, PeriodTimes);
  FinalResult FR = CalcBaseFinalResult(TotalCost, Launches, PeriodTimes);
  return FR;
}