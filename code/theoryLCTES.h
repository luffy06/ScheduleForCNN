const int REPEATLIMITED = 20;
int REPEAT = 2;

struct NodeComparationByCost {
  bool operator() (const Node &a, const Node &b) const {
    if (a.TopoOrder != b.TopoOrder)
      return a.TopoOrder < b.TopoOrder;
    else if (a.Cost != b.Cost)
      return a.Cost < b.Cost;
    return a.Id > b.Id;    
  }
};

struct NodeComparationByOutEdge {
  bool operator() (const Node &a, const Node &b) const {
    if (a.TopoOrder != b.TopoOrder)
      return a.TopoOrder > b.TopoOrder;
    else if (a.MaxOutEdge - a.Cost != b.MaxOutEdge - b.Cost)
      return a.MaxOutEdge - a.Cost < b.MaxOutEdge - b.Cost;
    return a.Id > b.Id;
  }
};

struct TimeInterval : PEInterval {
  TimeInterval(int a, long long b, long long c) : PEInterval(a, b, c) {
  }

  friend bool operator< (TimeInterval a, TimeInterval b) {
    if (a.EndTime != b.EndTime)
      return a.EndTime > b.EndTime;
    return a.PEId > b.PEId;
  }
};

struct Phase {
  int PENumb;
  int TotalNode;
  vector<PEInterval> PETimes;
  double Ratio;

  Phase() { }

  Phase(int a, int b) {
    Ratio = 0.0;
    PENumb = a;
    TotalNode = b;
    PETimes.clear();
  }

  void SortPETimes() {
    sort(PETimes.begin(), PETimes.end());
  }
};

struct Iteration {
  int PENumb;
  int PhasePE;
  int TotalNode;
  long long UpBound;
  long long Prelogue;
  int Retiming;
  int RunOnCache;
  int RunOnDRAM;

  Iteration(int a, int b, int c) {
    UpBound = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    PENumb = a;
    PhasePE = b;
    TotalNode = c;
  }

  void CalcPrelogue(Node NodeTime[REPEATLIMITED + 1][MAXN]) {
    int MinRetiming = INF;
    int MaxRetiming = -1;
    for (int i = 1; i <= TotalNode; ++ i) {
      for (int j = 1; j <= 2 * REPEAT; ++ j) {
        int Retiming = NodeTime[j][i].Retiming;
        MinRetiming = min(MinRetiming, Retiming);
        MaxRetiming = max(MaxRetiming, Retiming);
      }
    }
    Retiming = MaxRetiming - MinRetiming + 1;
    Prelogue = Retiming * UpBound;
  }
};

vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
Node NodeList[MAXN];
Node NodeTime[REPEATLIMITED + 1][MAXN];
Node IterNodeTime[REPEATLIMITED * 2 + 1][MAXN];
int DP[MAXN][MAXSIZE + 1];
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;

vector<CacheManager> Caches;
vector<CacheBlock> DRAMBlocks;

vector<Iteration> IterList;

bool ReChecked[MAXN][REPEATLIMITED * 2 + 1];

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

bool CmpByTime(Node a, Node b) {
  if (a.StartTime != b.StartTime)
    return a.StartTime < b.StartTime;
  return a.EndTime < b.EndTime;
}

void Show(Node NodeTable[][MAXN], int TotalRound) {
  vector<Node> PELine[MAXPE];
  int PENumb = -1;
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 1; j <= TotalRound; ++ j) {
      Node node = NodeTable[j][i];
      node.Show();
      PELine[node.PEId].push_back(node);
      PENumb = max(PENumb, node.PEId);
    }
  }
  for (int i = 1; i <= PENumb; ++ i) {
    sort(PELine[i].begin(), PELine[i].end(), CmpByTime);
    int NowTime = 0;
    for (int j = 0; j < PELine[i].size(); ++ j) {
      for (int k = NowTime; k < PELine[i][j].StartTime; ++ k)
        printf("--");
      for (int k = PELine[i][j].StartTime; k < PELine[i][j].EndTime; ++ k)
        printf("%c%d", 'A' + PELine[i][j].Id - 1, PELine[i][j].Round);
      NowTime = PELine[i][j].EndTime;
    }
    printf("\n");
  }
}

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
    BinSize = Sum - BinSize;
    // printf("### Bad BinSize ###\n");
    // printf("Good Size:%lu\n", Goods.size());
    // printf("BinSize:%d\tSum:%d\n", BinSize, Sum);
  }
  assert(BinSize <= MAXSIZE);
  assert(Goods.size() < MAXN);

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

long long GetStartTime(long long StartTime, int Round, int NodeId, int PEId, Phase &phase) {
  vector<Edge> Edges = ReEdgeList[NodeId];
  if (Edges.size() > 0) {
    for (int j = 0; j < Edges.size(); ++ j) {
      Edge e = Edges[j];
      assert(NodeTime[Round][e.From].EndTime != -1);
      StartTime = max(StartTime, Ceil(e.Memory, CACHESPEED) + NodeTime[Round][e.From].EndTime);
    }
    for (int j = 0; j < Edges.size(); ++ j) {
      Edge e = Edges[j];
      CacheBlock CB = CacheBlock(e.From, Round, e.To, Round, e.Memory, NodeTime[Round][e.From].EndTime, StartTime);
      assert(PEId - 1 >= 0 && PEId - 1 < Caches.size());
      Caches[PEId - 1].AddCacheBlock(CB);
    }
  }
  return StartTime;
}

void InitPhase(Phase &phase) {
  // vector<TimeInterval> IntervalQue;
  priority_queue<TimeInterval> IntervalQue;
  priority_queue<Node, vector<Node>, NodeComparationByOutEdge> NodeQue;
  int PENumb = phase.PENumb;
  for (int i = 1; i <= PENumb; ++ i) {
    TimeInterval Interval = TimeInterval(i, 0, 0);
    // IntervalQue.push_back(Interval);
    IntervalQue.push(Interval);
    CacheManager CM = CacheManager();
    CM.PEId = i;
    Caches.push_back(CM);
  }
  assert(PENumb > 0);
  int Index = 1;
  int NowOrder = -1;
  long long MaxEndtime = 0;
  do {
    queue<TimeInterval> TempQue;
    while (!IntervalQue.empty()) {
      TimeInterval TI = IntervalQue.top();
      IntervalQue.pop();
      TI.SetTime(TI.StartTime, MaxEndtime);
      TempQue.push(TI);
    }
    while (!TempQue.empty()) {
      TimeInterval TI = TempQue.front();
      TempQue.pop();
      IntervalQue.push(TI);      
    }
    for (; Index <= TotalNode && NodeList[Index].TopoOrder == NowOrder; ++ Index)
      NodeQue.push(NodeList[Index]);
    int PEIndex = 0;
    while (!NodeQue.empty()) {
      Node node = NodeQue.top();
      NodeQue.pop();
      for (int k = 1; k <= REPEAT; ++ k) {
        TimeInterval TI = IntervalQue.top();
        IntervalQue.pop();
        long long StartTime = GetStartTime(TI.EndTime, k, node.Id, TI.PEId, phase);
        node.Retiming = 0;
        node.Round = k;
        node.PEId = TI.PEId;
        node.SetTime(StartTime, StartTime + node.Cost);
        NodeTime[node.Round][node.Id].Copy(node);
        TI.SetTime(TI.StartTime, node.EndTime);
        MaxEndtime = max(MaxEndtime, node.EndTime);
        IntervalQue.push(TI);
      }
      // if (PEIndex == IntervalQue.size())
      //   PEIndex = 0;
      // bool Horizontal = NodeQue.size() >= PENumb;
      // if (!Horizontal) {
      //   Node node = NodeQue.top();
      //   NodeQue.pop();
      //   for (int k = 1; k <= REPEAT; ++ k, ++ PEIndex) {
      //     if (PEIndex == IntervalQue.size())
      //       PEIndex = 0;
      //     long long StartTime = GetStartTime(IntervalQue[PEIndex].EndTime, k, node.Id, IntervalQue[PEIndex].PEId, phase);
      //     node.PEId = IntervalQue[PEIndex].PEId;
      //     node.Round = k;
      //     node.SetTime(StartTime, StartTime + node.Cost);
      //     NodeTime[node.Round][node.Id].Copy(node);
      //     IntervalQue[PEIndex].SetTime(IntervalQue[PEIndex].StartTime, node.EndTime);
      //   }
      // }
      // else {
      //   for (; PEIndex < IntervalQue.size(); ++ PEIndex) {
      //     Node node = NodeQue.top();
      //     NodeQue.pop();
      //     for (int k = 1; k <= REPEAT; ++ k) {
      //       long long StartTime = GetStartTime(IntervalQue[PEIndex].EndTime, k, node.Id, IntervalQue[PEIndex].PEId, phase);
      //       if (k > 1)
      //         StartTime = max(StartTime, NodeTime[k - 1][node.Id].EndTime);
      //       node.PEId = IntervalQue[PEIndex].PEId;
      //       node.Round = k;
      //       node.SetTime(StartTime, StartTime + node.Cost);
      //       NodeTime[node.Round][node.Id].Copy(node);
      //       IntervalQue[PEIndex].SetTime(IntervalQue[PEIndex].StartTime, node.EndTime);
      //     }
      //   }
      // }
    }
    if (Index <= TotalNode)
      NowOrder = NodeList[Index].TopoOrder;
  } while (Index <= TotalNode);
  phase.PETimes.push_back(PEInterval(0, -1, -1));
  // for (int i = 0; i < IntervalQue.size(); ++ i) {
  //   TimeInterval TI = IntervalQue[i];
  //   phase.PETimes.push_back(PEInterval(TI.PEId, TI.StartTime, TI.EndTime));
  // }
  while (!IntervalQue.empty()) {
    TimeInterval TI = IntervalQue.top();
    IntervalQue.pop();
    phase.PETimes.push_back(PEInterval(TI.PEId, TI.StartTime, TI.EndTime));
  }
  phase.SortPETimes();

  MaxEndtime = 0;
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= REPEAT; ++ j)
      TotalCost = TotalCost + NodeTime[j][i].Cost;
  for (int i = 1; i <= PENumb; ++ i)
    MaxEndtime = max(MaxEndtime, phase.PETimes[i].EndTime);
  phase.Ratio = TotalCost / (1.0 * MaxEndtime * PENumb);
  printf("Ratio of Phase:%.6f\n", phase.Ratio);
}

int Init(int TotalPE) {
  memset(Degree, 0, sizeof(Degree));
  memset(ReChecked, false, sizeof(ReChecked));

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
      NodeList[e.From].MaxOutEdge = max(NodeList[e.From].MaxOutEdge, e.Memory);
    }
  }

  int NeedPE = GetTopology();
  sort(NodeList + 1, NodeList + 1 + TotalNode, CmpByTopoOrder);

  int MaxRepeat = 0, MinRepeat = INF, SumRepeat = 0, RepeatCount = 0;
  for (int i = 1; i <= TotalNode; ++ i) {
    long long Sum = 0;
    int Count = 0;
    vector<Edge> Edges = EdgeList[NodeList[i].Id];
    for (int j = 0; j < Edges.size(); ++ j) {
      Edge e = Edges[j];
      if (Ceil(e.Memory, CACHESPEED) > 0) {
        Sum = Sum + Ceil(e.Memory, CACHESPEED);
        Count = Count + 1;
      }
    }
    if (Count == 0)
      continue;
    int NowRepeat = Ceil(Sum, (1LL * Count * NodeList[i].Cost));
    MaxRepeat = max(MaxRepeat, NowRepeat);
    MinRepeat = min(MinRepeat, NowRepeat);
    SumRepeat = SumRepeat + NowRepeat;
    RepeatCount = RepeatCount + 1;
  }
  if (MaxRepeat == 0)
    REPEAT = 2;
  else
    REPEAT = (MaxRepeat + MinRepeat) / 2;
  REPEAT = min(REPEAT, REPEATLIMITED);
  REPEAT = max(REPEAT, 2);
  printf("Min:%d\tMAX:%d\tREPEAT:%d\tAverage Repeat:%d\n", MinRepeat, MaxRepeat, 
                      REPEAT, (RepeatCount == 0 ? 0 : SumRepeat / RepeatCount));

  if (TotalPE >= NeedPE) {
    IterList.push_back(Iteration(NeedPE, NeedPE, TotalNode));
    if (TotalPE % NeedPE > 0)
      IterList.push_back(Iteration(TotalPE % NeedPE, TotalPE % NeedPE, TotalNode));
  }
  else {
    IterList.push_back(Iteration(TotalPE, TotalPE, TotalNode));
  }
  return NeedPE;
}

void DetectCacheOverflow(Iteration &iteration) {
  for (int i = 1; i <= iteration.PENumb; ++ i) {
    assert(i - 1 >= 0 && i - 1 < Caches.size());
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    // printf("PE:%d/%d\tTimeTrace Size:%lu\n", i, iteration.PENumb, TimeTrace.size());
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      long long ST = TimeTrace[j];
      long long ED = TimeTrace[j + 1];
      vector<CacheBlock> Blocks;
      Caches[i - 1].GetCacheBlockByTime(ST, ED, Blocks);
      if (Blocks.size() == 0)
        continue;

      vector<int> Memory;
      for (int k = 0; k < Blocks.size(); ++ k)
        Memory.push_back(Blocks[k].Memory);
      vector<int> ArrangedSet = ArrangeInFixedSize(Memory, CACHESIZE);
      
      for (int k = ArrangedSet.size() - 1; k >= 0; -- k)
        Blocks.erase(Blocks.begin() + ArrangedSet[k]);

      iteration.RunOnDRAM = iteration.RunOnDRAM + Blocks.size();
      iteration.RunOnCache = iteration.RunOnCache - Blocks.size();
      for (int k = 0; k < Blocks.size(); ++ k) {
        CacheBlock CB = Blocks[k];
        Caches[i - 1].DeleteCacheBlock(CB);
        DRAMBlocks.push_back(CB);
        ReChecked[CB.NodeIds.second][CB.Rounds.second] = true;
      }
    }
  }
}

bool GetStrogePos(int FromId, int FromRound, int ToId, int ToRound) {
  for (int i = 0; i < DRAMBlocks.size(); ++ i) {
    CacheBlock CB = DRAMBlocks[i];
    if (CB.NodeIds.first == FromId && CB.NodeIds.second == ToId 
      && CB.Rounds.first == FromRound && CB.Rounds.second == ToRound) 
      return false;
  }
  return true;
}

void ReBFS(Node KeyNode, Iteration &iteration) {
  // printf("ReBFS\n");
  long long PeriodTime = iteration.UpBound;
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  q.push(KeyNode);
  while (!q.empty()) {
    Node ToNode = q.top();
    q.pop();
    // ToNode.Show();
    ReChecked[ToNode.Id][ToNode.Round] = false;

    vector<Edge> InEdges = ReEdgeList[ToNode.Id];
    sort(InEdges.begin(), InEdges.end(), CmpEdgeByFromCost);
    for (int i = 0; i < InEdges.size(); ++ i) {
      Edge e = InEdges[i];
      // printf("From:%d To:%d %d %d\n", InEdges[i].From, InEdges[i].To, ToNode.Id, ToNode.Round);
      Node FromNode = IterNodeTime[ToNode.Round][InEdges[i].From];
      long long Cost = (GetStrogePos(FromNode.Id, FromNode.Round, ToNode.Id, ToNode.Round) 
                      ? Ceil(e.Memory, CACHESPEED) : Ceil(e.Memory, DRAMSPEED));
      if (FromNode.EndTime + FromNode.Retiming * PeriodTime + Cost > ToNode.StartTime + ToNode.Retiming * PeriodTime) {
        // FE + Retiming * P <= TS
        FromNode.Retiming = Floor(ToNode.StartTime + ToNode.Retiming * PeriodTime - FromNode.EndTime, PeriodTime);
        IterNodeTime[FromNode.Round][FromNode.Id] = FromNode;
        q.push(FromNode);
      }
    }
  }
}

void InitIteration(Iteration &iteration) {
  int PhasePE = iteration.PhasePE;
  Phase phase = Phase(PhasePE, TotalNode);
  // printf("Init Phase\n");
  InitPhase(phase);
  // printf("Init Iteration\n");

  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= REPEAT; ++ j)
      IterNodeTime[j][i] = NodeTime[j][i];

  int Shift = iteration.PENumb - phase.PENumb;
  printf("Shift:%d\n", Shift);
  long long MaxEndtime = 0;
  vector<int> Moves;
  Moves.push_back(0);
  // printf("Calc Shift Out Part\n");
  for (int i = 1; i <= Shift; ++ i) {
    long long Offset = 0;
    Moves.push_back(Offset);
    iteration.UpBound = max(iteration.UpBound, phase.PETimes[i].EndTime + Offset);
    
    CacheManager CM = Caches[i - 1];
    for (int j = 0; j < CM.Cache.size(); ++ j) {
      CM.Cache[j].Rounds = make_pair(CM.Cache[j].Rounds.first + REPEAT, CM.Cache[j].Rounds.second + REPEAT);
      CM.Cache[j].StartTime = CM.Cache[j].StartTime + Offset;
      CM.Cache[j].EndTime = CM.Cache[j].EndTime + Offset;
    }
    Caches.push_back(CM);
  }
  // printf("Calc Shift In Part\n");
  for (int i = Shift + 1; i <= phase.PENumb; ++ i) {
    int j = phase.PENumb + Shift + 1 - i;
    int Offset = phase.PETimes[j].EndTime - phase.PETimes[i].StartTime;
    Offset = max(Offset, Moves[Moves.size() - 1]);
    Moves.push_back(Offset);
    iteration.UpBound = max(iteration.UpBound, phase.PETimes[i].EndTime + Offset);
    
    CacheManager CM = Caches[i - 1];
    for (int k = 0; k < CM.Cache.size(); ++ k) {
      CacheBlock CB = CM.Cache[k];
      CB.Rounds = make_pair(CB.Rounds.first + REPEAT, CB.Rounds.second + REPEAT);
      CB.StartTime = CB.StartTime + Offset;
      CB.EndTime = CB.EndTime + Offset;
      Caches[j - 1].AddCacheBlock(CB);
    }
  }

  // printf("Calc Second Phase\n");
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 1; j <= REPEAT; ++ j) {
      Node node = NodeTime[j][i];
      int PEId = phase.PENumb + Shift - node.PEId + 1;
      int Round = j + REPEAT;
      int Offset = Moves[node.PEId];
      long long StartTime = node.StartTime + Offset;
      long long EndTime = node.EndTime + Offset;
      node.Retiming = 0;
      node.PEId = PEId;
      node.Round = Round;
      node.SetTime(StartTime, EndTime);
      IterNodeTime[node.Round][node.Id] = node;
      MaxEndtime = max(MaxEndtime, EndTime);
    }
  }

  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= REPEAT; ++ j)
      TotalCost = TotalCost + NodeTime[j][i].Cost * 2;
  double Ratio = TotalCost / (1.0 * MaxEndtime * iteration.PENumb);
  printf("Ratio of Iteration:%.6f\n", Ratio);
  printf("UpBound:%lld\n", iteration.UpBound);

  assert(iteration.UpBound > 0);
  for (int i = 0; i < Caches.size(); ++ i)
    iteration.RunOnCache = iteration.RunOnCache + Caches[i].Cache.size();

  // printf("Detect Cache Overflow\n");
  DetectCacheOverflow(iteration);

  vector<Node> ReCheckedNodes;
  for (int i = 1; i <= TotalNode; ++ i) 
    for (int j = 1; j <= REPEAT * 2; ++ j) 
      if (ReChecked[i][j]) ReCheckedNodes.push_back(IterNodeTime[j][i]);
  
  sort(ReCheckedNodes.begin(), ReCheckedNodes.end(), CmpByTopoOrder);
  // printf("ReChecked Nodes\n");
  for (int i = ReCheckedNodes.size() - 1; i >= 0; -- i) {
    Node KeyNode = ReCheckedNodes[i];
    if (ReChecked[KeyNode.Id][KeyNode.Round])
      ReBFS(IterNodeTime[KeyNode.Round][KeyNode.Id], iteration);
  }

  iteration.CalcPrelogue(IterNodeTime);
}

FinalResult CalcResult(int TotalPE, int NeedPE, int PeriodTimes) {
  FinalResult FR = FinalResult();
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;

  for (int i = 0; i < IterList.size(); ++ i) {
    Caches.clear();
    DRAMBlocks.clear();
    InitIteration(IterList[i]);
  }

  if (IterList.size() == 1) {
    Iteration iteration = IterList[0];
    int Launches = Ceil(TotalPE, NeedPE);
    int X = Ceil(Ceil(PeriodTimes, Launches), 2 * REPEAT);
    FR.TotalTime = iteration.Prelogue + 1LL * max(0, X - 1) * iteration.UpBound;
    FR.Prelogue = iteration.Prelogue;
    FR.Retiming = iteration.Retiming;
    FR.RunOnCache = X * iteration.RunOnCache * Launches;
    FR.RunOnDRAM = X * iteration.RunOnDRAM * Launches;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    assert(IterList.size() == 2);
    Iteration iterationX = IterList[0];
    Iteration iterationY = IterList[1];
    int Launches = TotalPE / iterationX.PENumb;
    for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
      int EachY = PeriodTimes - EachX;
      int X = Ceil(Ceil(EachX, Launches), 2 * REPEAT);
      int Y = Ceil(EachY, 2 * REPEAT);
      long long TotalTimeX = iterationX.Prelogue + 1LL * max(0, X - 1) * iterationX.UpBound;
      long long TotalTimeY = iterationY.Prelogue + 1LL * max(0, Y - 1) * iterationY.UpBound;
      long long TotalTime = max(TotalTimeX, TotalTimeY);
      if (FR.TotalTime == -1 || TotalTime < FR.TotalTime) {
        FR.TotalTime = TotalTime;
        FR.RunOnCache = iterationX.RunOnCache * X * Launches + iterationY.RunOnCache * Y;
        FR.RunOnDRAM = iterationX.RunOnDRAM * X * Launches + iterationY.RunOnDRAM * Y;
      }
    }
    FR.Retiming = iterationX.Retiming;
    FR.Prelogue = iterationX.Prelogue;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  return FR;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  int NeedPE = Init(TotalPE);
  FinalResult FR = CalcResult(TotalPE, NeedPE, PeriodTimes);
  return FR;
}