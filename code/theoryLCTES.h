const int REPEATLIMITED = 20;
int REPEAT = 2;

struct Node {
  int Id;
  int Cost;

  int InDegree;
  int OutDegree;
  int TopoOrder;

  int PEId;
  int Round;
  int Retiming;
  int StartTime;
  int EndTime;

  int MaxOutEdge;

  Node() {
    Id = -1;
    Cost = 0;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
    PEId = -1;
    Round = -1;
    Retiming = 0;
    StartTime = EndTime = -1;
    MaxOutEdge = 0;
  }

  void SetTime(int st) {
    StartTime = st;
    EndTime = StartTime + Cost;
    assert(StartTime <= EndTime);
  }

  void Copy(Node t) {
    Id = t.Id;
    Cost = t.Cost;
    InDegree = t.InDegree;
    OutDegree = t.OutDegree;
    TopoOrder = t.TopoOrder;
    PEId = t.PEId;
    Round = t.Round;
    Retiming = t.Retiming;
    StartTime = t.StartTime;
    EndTime = t.EndTime;
    MaxOutEdge = t.MaxOutEdge;
  }

  void Show() {
    printf("ID:%d\tPE:%d\tRound:%d\tRetiming:%d\tST:%d\tED:%d\tCost:%d\tTopoOrder:%d\n", Id, PEId, Round, Retiming, StartTime, EndTime, Cost, TopoOrder);
  }
};

struct Edge {
  int From;
  int To;
  int Memory;
  int CacheTimeCost;
  int DRAMTimeCost;

  Edge() { }

  Edge(int a, int b, int c) {
    From = a;
    To = b;
    Memory = c;
    CacheTimeCost = ceil(1.0 * Memory / CACHESPEED);
    DRAMTimeCost = ceil(1.0 * Memory / DRAMSPEED);
  }

  void Show() {
    printf("From:%d\tTo:%d\tMemory:%d\tCacheTimeCost:%d\tDRAMTimeCost:%d\n", From, To, Memory, CacheTimeCost, DRAMTimeCost);
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

struct Phase {
  int PENumb;
  vector<Node> PELine[MAXPE];
  int PEEndTime[MAXPE];
  int PEStartTime[MAXPE];
  int RunOnDRAM;
  int RunOnCache;
  double Ratio;

  Phase() { }

  Phase(int a) {
    Ratio = 0.0;
    PENumb = a;
    RunOnDRAM = RunOnCache = 0;
  }

  void Show() {
    for (int i = 1; i <= PENumb; ++ i) {
      printf("PE:%d\t", i);
      int LastStartTime = 0;
      for (int j = 0; j < PELine[i].size(); ++ j) {
        Node node = PELine[i][j];
        if (node.StartTime > LastStartTime) {
          for (int k = LastStartTime; k < node.StartTime; ++ k)
            printf("-");
        }
        for (int k = node.StartTime; k < node.EndTime; ++ k) {
          printf("%c", ('A' + node.Id - 1));
        }
        LastStartTime = node.EndTime;
      }
      printf("\n");
    }
  }

  void CalcRatio() {
    int MaxEndtime = 0;
    double TotalCost = 0;
    for (int i = 1; i <= PENumb; ++ i) {
      MaxEndtime = max(MaxEndtime, PEEndTime[i]);
      for (int j = 0; j < PELine[i].size(); ++ j)
        TotalCost = TotalCost + PELine[i][j].Cost;
    }
    Ratio = TotalCost / (1.0 * MaxEndtime * PENumb);
  }
};

struct Iteration {
  int PENumb;
  int Shift;
  Phase phase;
  vector<int> Moves;
  long long Cost;
  int Cross;
  int RunOnCache;
  int RunOnDRAM;

  Iteration(Phase OnePhase, int TotalPE) {
    phase = OnePhase;
    Cross = INF;
    Cost = 0;
    RunOnCache = phase.RunOnCache * 2;
    RunOnDRAM = phase.RunOnDRAM * 2;
    Shift = TotalPE - phase.PENumb;
    PENumb = TotalPE;
    printf("Shift:%d\n", Shift);

    Moves.push_back(0);
    for (int i = 1; i <= Shift; ++ i) {
      Moves.push_back(phase.PEStartTime[phase.PENumb]);
      Cost = max(Cost, 1LL * phase.PEEndTime[i] + phase.PEStartTime[phase.PENumb]);
    }
    for (int i = Shift + 1; i <= phase.PENumb; ++ i) {
      int j = phase.PENumb + Shift + 1 - i;
      int Offset = phase.PEEndTime[j] - phase.PEStartTime[i];
      Offset = max(Offset, Moves[Moves.size() - 1]);
      Moves.push_back(Offset);
      Cost = max(Cost, 1LL * phase.PEEndTime[i] + Offset);
    }

    CalcCross(phase);
    printf("Cross:%d\n", Cross);
    assert(Cost > 0);
  }

  void CalcCross(Phase phase) {
    for (int i = 1; i <= Shift; ++ i) {
      int StartTime = phase.PEStartTime[i];
      int EndTime = phase.PEEndTime[i];
      Cross = min(Cross, (int)(Cost - EndTime + StartTime));
    }
    for (int i = Shift + 1; i <= phase.PENumb; ++ i) {
      int j = phase.PENumb + Shift + 1 - i;
      int StartTime = phase.PEStartTime[j];
      int EndTime = phase.PEEndTime[i] + Moves[i];
      Cross = min(Cross, (int)(Cost - EndTime + StartTime));
    }
  }

  void Show() {
    for (int i = 1; i <= PENumb; ++ i) {
      printf("PE:%d\t", i);
      int LastStartTime = 0;
      if (i <= phase.PENumb) {
        for (int j = 0; j < phase.PELine[i].size(); ++ j) {
          Node node = phase.PELine[i][j];
          int StartTime = node.StartTime;
          int EndTime = node.EndTime;
          if (StartTime > LastStartTime) {
            for (int k = LastStartTime; k < StartTime; ++ k)
              printf("-");
          }
          for (int k = StartTime; k < EndTime; ++ k)
            printf("%c", ('A' + node.Id - 1));
          LastStartTime = EndTime;
        }
      }
      if (i > Shift) {
        int End = phase.PENumb - i + Shift + 1;
        int Offset = Moves[End];
        for (int j = 0; j < phase.PELine[End].size(); ++ j) {
          Node node = phase.PELine[End][j];
          int StartTime = node.StartTime + Offset;
          int EndTime = node.EndTime + Offset;
          if (StartTime > LastStartTime) {
            for (int k = LastStartTime; k < StartTime; ++ k)
              printf("-");
          }
          for (int k = StartTime; k < EndTime; ++ k)
            printf("%c", ('A' + node.Id - 1));
          LastStartTime = EndTime;
        }
      }
      printf("\n");
    }
  }
};

vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
Node NodeList[MAXN];
Node NodeTime[MAXPE][MAXN];
int Degree[MAXN];
int DP[MAXN][MAXSIZE];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;

bool CmpByTopoOrder(Node a, Node b) {
  if (a.TopoOrder != b.TopoOrder)
    return a.TopoOrder < b.TopoOrder;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
}

int GetTopology() {
  int Count = 0, Order = 0;
  int NeedPE = 0;
  queue<Node> q;
  for (int i = 1; i <= TotalNode; ++ i) {
    if (Degree[i] == 0) {
      q.push(NodeList[i]);
    }
  }
  Count = NeedPE = q.size();

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
      Count = q.size();
      Order = Order + 1;
    }
  }
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

int GetStartTime(int StartTime, int k, int NodeId, Phase &phase) {
  vector<Edge> Edges = ReEdgeList[NodeId];
  if (Edges.size() > 0) {
    vector<int> NodeSizes;
    for (int j = 0; j < Edges.size(); ++ j)
      NodeSizes.push_back(Edges[j].Memory);
    vector<int> ArrangeSet = ArrangeInFixedSize(NodeSizes, CACHESIZE);
    
    phase.RunOnCache = phase.RunOnCache + ArrangeSet.size();
    phase.RunOnDRAM = phase.RunOnDRAM + Edges.size() - ArrangeSet.size();

    for (int j = ArrangeSet.size() - 1; j >= 0; -- j) {
      if (j > 0) assert(ArrangeSet[j] > ArrangeSet[j - 1]);
      Edge e = Edges[ArrangeSet[j]];

      StartTime = max(StartTime, e.CacheTimeCost + NodeTime[k][e.From].EndTime);
      Edges.erase(Edges.begin() + ArrangeSet[j]);
    }

    for (int j = 0; j < Edges.size(); ++ j) {
      Edge e = Edges[j];
      StartTime = max(StartTime, e.DRAMTimeCost + NodeTime[k][e.From].EndTime);
    }
  }
  return StartTime;
}

void InitPhase(Phase &phase) {
  int PENumb = phase.PENumb;
  for (int i = 1; i <= PENumb; ++ i) {
    phase.PEStartTime[i] = INF;
    phase.PEEndTime[i] = 0;
  }
  priority_queue<Node, vector<Node>, NodeComparationByOutEdge> WaitingQue;
  int Index = 1;
  int NowOrder = -1;
  do {
    for (; Index <= TotalNode && NodeList[Index].TopoOrder == NowOrder; ++ Index)
      WaitingQue.push(NodeList[Index]);
    int PEIter = 1;
    while (!WaitingQue.empty()) {
      if (PEIter == PENumb + 1)
        PEIter = 1;
      bool Horizontal = WaitingQue.size() >= PENumb;
      
      if (!Horizontal) {
        Node node = WaitingQue.top();
        WaitingQue.pop();
        for (int k = 0; k < REPEAT; ++ k, PEIter = PEIter + 1) {
          if (PEIter == PENumb + 1)
            PEIter = 1;
          int StartTime = GetStartTime(phase.PEEndTime[PEIter], k, node.Id, phase);
          node.PEId = PEIter;
          node.Round = k;
          node.SetTime(StartTime);
          NodeTime[node.Round][node.Id].Copy(node);
          phase.PELine[PEIter].push_back(node);
          phase.PEStartTime[PEIter] = min(phase.PEStartTime[PEIter], node.StartTime);
          phase.PEEndTime[PEIter] = max(phase.PEEndTime[PEIter], node.EndTime);
          assert(phase.PEStartTime[PEIter] >= 0);
          assert(phase.PEEndTime[PEIter] >= 0);
        }
      }
      else {
        for (; PEIter <= PENumb; ++ PEIter) {
          Node node = WaitingQue.top();
          WaitingQue.pop();
          for (int k = 0; k < REPEAT; ++ k) {
            int StartTime = GetStartTime(phase.PEEndTime[PEIter], k, node.Id, phase);
            if (k > 0)
              StartTime = max(StartTime, NodeTime[k - 1][node.Id].EndTime);
            node.PEId = PEIter;
            node.Round = k;
            node.SetTime(StartTime);
            NodeTime[node.Round][node.Id].Copy(node);
            phase.PELine[PEIter].push_back(node);
            phase.PEStartTime[PEIter] = min(phase.PEStartTime[PEIter], node.StartTime);
            phase.PEEndTime[PEIter] = max(phase.PEEndTime[PEIter], node.EndTime);
            assert(phase.PEStartTime[PEIter] >= 0);
            assert(phase.PEEndTime[PEIter] >= 0);
          }
        }
      }
    }
    if (Index <= TotalNode)
      NowOrder = NodeList[Index].TopoOrder;
  } while (Index <= TotalNode);
  phase.CalcRatio();
  printf("Ratio of Phase:%.6f\n", phase.Ratio);
}

int Init(int TotalPE) {
  memset(Degree, 0, sizeof(Degree));

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
      if (Edges[j].CacheTimeCost > 0) {
        Sum = Sum + Edges[j].CacheTimeCost;
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
  printf("Min:%d\tMAX:%d\tREPEAT:%d\tAverage Repeat:%d\n", MinRepeat, MaxRepeat, REPEAT, (RepeatCount == 0 ? 0 : SumRepeat / RepeatCount));
  return NeedPE;
}

Iteration InitIteration(int NeedPE, int TotalPE) {  
  Phase phase = Phase(NeedPE);
  InitPhase(phase);
  Iteration iteration = Iteration(phase, TotalPE);
  return iteration;
}

long long CalcTotalTime(int Iter, long long Cost, int Launch, int Cross) {
  int IterationTimes = Ceil(Ceil(Iter, Launch), 2 * REPEAT);
  return IterationTimes * Cost - (IterationTimes - 1) * Cross;
}

FinalResult CalcResult(int TotalPE, int NeedPE, int PeriodTimes) {
  FinalResult FR = FinalResult();
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;
  int PhasePE = NeedPE;
  int IterationPE = PhasePE + 1;
  for (; IterationPE >= NeedPE / 2; -- IterationPE) {
    if (TotalPE % IterationPE == 0) {
      PhasePE = IterationPE - 1;
      break;
    }
  }
  if (TotalPE % IterationPE != 0)
    IterationPE = PhasePE + 1;
  // IterationPE = PhasePE;
  printf("PhasePE:%d\tIterationPE:%d\n", PhasePE, IterationPE);

  if (TotalPE >= IterationPE) {
    int Launches = TotalPE / IterationPE;
    Iteration iteration = InitIteration(PhasePE, IterationPE);
    int IterationTimes = Ceil(Ceil(PeriodTimes, Launches), 2 * REPEAT);
    FR.Prelogue = 0;
    FR.Retiming = 0;
    FR.RunOnCache = IterationTimes * iteration.RunOnCache * Launches;
    FR.RunOnDRAM = IterationTimes * iteration.RunOnDRAM * Launches;
    FR.TotalTime = CalcTotalTime(PeriodTimes, iteration.Cost, Launches, iteration.Cross);
    if (TotalPE % IterationPE > 0) {
      Iteration iterationrest = InitIteration(TotalPE % IterationPE, TotalPE % IterationPE);
      for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
        int EachY = PeriodTimes - EachX;
        long long TotalTimeX = CalcTotalTime(EachX, iteration.Cost, Launches, iteration.Cross);
        long long TotalTimeY = CalcTotalTime(EachY, iterationrest.Cost, 1, iterationrest.Cross);
        long long TotalTime = max(TotalTimeX, TotalTimeY);
        if (FR.TotalTime == -1 || TotalTime < FR.TotalTime) {
          FR.TotalTime = TotalTime;
          int IterationTimesX = Ceil(Ceil(EachX, Launches), 2 * REPEAT);
          int IterationTimesY = Ceil(Ceil(EachY, Launches), 2 * REPEAT);
          FR.RunOnCache = iteration.RunOnCache * IterationTimesX * Launches + iterationrest.RunOnCache * IterationTimesY;
          FR.RunOnDRAM = iteration.RunOnDRAM * IterationTimesX * Launches + iterationrest.RunOnDRAM * IterationTimesY;
        }
      }
    }
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    Iteration iteration = InitIteration(TotalPE, TotalPE);
    int IterationTimes = Ceil(PeriodTimes, 2 * REPEAT);
    FR.TotalTime = CalcTotalTime(PeriodTimes, iteration.Cost, 1, iteration.Cross);
    FR.Prelogue = 0;
    FR.Retiming = 0;
    FR.RunOnCache = IterationTimes * iteration.RunOnCache;
    FR.RunOnDRAM = IterationTimes * iteration.RunOnDRAM;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  return FR;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  int NeedPE = Init(TotalPE);
  FinalResult FR = CalcResult(TotalPE, NeedPE, PeriodTimes);
  return FR;
}