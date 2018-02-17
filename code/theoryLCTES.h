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

  Node() {
    Id = -1;
    Cost = 0;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
    PEId = -1;
    Round = -1;
    Retiming = 0;
    StartTime = EndTime = -1;
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
};

struct Phase {
  int PENumb;
  vector<Node> PELine[MAXPE];
  int PEEndTime[MAXPE];
  int PEStartTime[MAXPE];
  int RunOnDRAM;
  int RunOnCache;

  Phase() { }

  Phase(int a) {
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
};

struct Iteration {
  int PENumb;
  int Shift;
  vector<Phase> PhaseGroup;
  vector<int> Moves;
  long long Cost;
  int Cross;
  int RunOnCache;
  int RunOnDRAM;

  Iteration(Phase phase, int TotalPE) {
    PhaseGroup.push_back(phase);
    PhaseGroup.push_back(phase);
    Cross = 0;
    Cost = 0;
    RunOnCache = phase.RunOnCache * 2;
    RunOnDRAM = phase.RunOnDRAM * 2;
    Shift = TotalPE - phase.PENumb;
    PENumb = TotalPE;

    for (int i = 1; i <= Shift; ++ i) {
      Moves.push_back(phase.PEStartTime[phase.PENumb]);
      Cost = max(Cost, 1LL * phase.PEEndTime[phase.PENumb] + phase.PEStartTime[phase.PENumb]);
    }
    for (int i = Shift + 1; i <= phase.PENumb; ++ i) {
      int j = phase.PENumb + Shift + 1 - i;
      int Offset = phase.PEEndTime[j] - phase.PEStartTime[i];
      if (Moves.size() > 0)
        Offset = max(Offset, Moves[Moves.size() - 1]);
      Moves.push_back(Offset);
      Cost = max(Cost, 1LL * phase.PEEndTime[phase.PENumb] + Offset);
    }

    CalcCross(phase);
  }

  void CalcCross(Phase phase) {
    for (int i = 1; i <= Shift; ++ i) {
      int StartTime = phase.PEStartTime[i];
      int EndTime = phase.PEEndTime[i];
      Cross = max(Cross, (int)(Cost - EndTime + StartTime));
    }
    for (int i = Shift + 1; i <= phase.PENumb; ++ i) {
      int j = phase.PENumb + Shift + 1 - i;
      int Offset = Moves[phase.PENumb - i + Shift];
      int StartTime = phase.PEStartTime[i];
      int EndTime = phase.PEEndTime[j] + Offset;
      Cross = max(Cross, (int)(Cost - EndTime + StartTime));
    }
  }

  void Show() {
    int PENumb = PhaseGroup[0].PENumb;
    for (int i = 1; i <= PENumb + Shift; ++ i) {
      printf("PE:%d\t", i);
      int LastStartTime = 0;
      if (i <= PENumb) {
        for (int j = 0; j < PhaseGroup[0].PELine[i].size(); ++ j) {
          Node node = PhaseGroup[0].PELine[i][j];
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
        int End = PENumb - i + Shift;
        int Offset = Moves[End];
        for (int j = 0; j < PhaseGroup[1].PELine[End + 1].size(); ++ j) {
          Node node = PhaseGroup[1].PELine[End + 1][j];
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
Node NodeTime[2][MAXN];
int Degree[MAXN];
int DP[MAXN][MAXSIZE];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;
int RunOnDRAM;
int RunOnCache;

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
    printf("### Bad BinSize ###\n");
    printf("Good Size:%lu\n", Goods.size());
    printf("BinSize:%d\tSum:%d\n", BinSize, Sum);
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

  // printf("MAXDP:%d\n", DP[Goods.size()][BinSize]);
  // for (int i = 0; i <= Goods.size(); ++ i) {
  //   if (i == 0)
  //     printf("0\t\t");
  //   else
  //     printf("%d\t\t", Goods[i - 1]);
  //   for (int j = 0; j <= BinSize; ++ j)
  //     printf("%d\t", DP[i][j]);
  //   printf("\n");
  // }
  sort(ArrangedGoods.begin(), ArrangedGoods.end());
  return ArrangedGoods;  
}

void InitPhase(Phase &phase) {
  int NowOrder = 0;
  int PEIter = 1;
  int PENumb = phase.PENumb;
  for (int i = 1; i <= PENumb; ++ i) {
    phase.PEStartTime[i] = INF;
    phase.PEEndTime[i] = 0;
  }
  for (int i = 1; i <= TotalNode; ++ i) {
    Node node = NodeList[i];
    if (node.TopoOrder != NowOrder) {
      PEIter = 1;
      NowOrder = node.TopoOrder;
    }

    for (int k = 0; k < 2; ++ k) {
      int StartTime = 0;
      vector<Edge> Edges = ReEdgeList[node.Id];
      if (Edges.size() > 0) {
        vector<int> NodeSizes;
        for (int j = 0; j < Edges.size(); ++ j)
          NodeSizes.push_back(Edges[j].Memory);
        vector<int> ArrangeSet = ArrangeInFixedSize(NodeSizes, CACHESIZE);
        
        phase.RunOnCache = phase.RunOnCache + ArrangeSet.size();
        phase.RunOnDRAM = phase.RunOnDRAM + Edges.size() - ArrangeSet.size();

        for (int j = ArrangeSet.size() - 1; j >= 0; -- j) {
          Edge e = Edges[ArrangeSet[j]];
          StartTime = max(StartTime, e.CacheTimeCost + NodeTime[k][e.From].EndTime);
          Edges.erase(Edges.begin() + j);
        }

        for (int j = 0; j < Edges.size(); ++ j) {
          Edge e = Edges[j];
          StartTime = max(StartTime, e.DRAMTimeCost + NodeTime[k][e.From].EndTime);
        }
      }
      if (k == 1)
        StartTime = max(StartTime, NodeTime[0][node.Id].EndTime);

      node.Round = k;
      node.SetTime(StartTime);
      NodeTime[node.Round][node.Id].Copy(node);
      phase.PELine[PEIter].push_back(node);
      if (k == 0)
        phase.PEStartTime[PEIter] = min(phase.PEStartTime[PEIter], node.StartTime);
      else
        phase.PEEndTime[PEIter] = max(phase.PEEndTime[PEIter], node.EndTime);
    }
    PEIter = PEIter + 1;

    if (PEIter == PENumb + 1) 
      PEIter = 1;
  }
}

int Init(int TotalPE) {
  memset(Degree, 0, sizeof(Degree));

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  RunOnDRAM = RunOnCache = 0;
  int NeedPE = GetTopology();
  return NeedPE;
}

Iteration InitIteration(int NeedPE, int TotalPE) {
  Phase phase = Phase(NeedPE);
  InitPhase(phase);
  Iteration iteration = Iteration(phase, TotalPE);
  return iteration;
}

long long CalcTotalTime(int Iter, long long Cost, int Launch, int Cross) {
  int IterationTimes = Ceil(Ceil(Iter, Launch), 4);
  return IterationTimes * Cost - (IterationTimes - 1) * Cross;
}

FinalResult CalcResult(int TotalPE, int NeedPE, int PeriodTimes) {
  FinalResult FR = FinalResult();
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;
  int IterationPE = NeedPE + 1;

  if (TotalPE >= IterationPE) {
    int Launches = TotalPE / IterationPE;
    Iteration iteration = InitIteration(NeedPE, IterationPE);
    int IterationTimes = Ceil(Ceil(PeriodTimes, Launches), 4);
    FR.Prelogue = 0;
    FR.Retiming = 0;
    FR.RunOnCache = IterationTimes * iteration.RunOnCache;
    FR.RunOnDRAM = IterationTimes * iteration.RunOnDRAM;
    FR.TotalTime = CalcTotalTime(PeriodTimes, iteration.Cost, Launches, iteration.Cross);
    if (TotalPE % IterationPE > 0) {
      Iteration iterationrest = InitIteration(TotalPE % IterationPE, TotalPE % IterationPE);
      for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
        int EachY = PeriodTimes - EachX;
        long long TotalTimeX = CalcTotalTime(EachX, iteration.Cost, Launches, iteration.Cross);
        long long TotalTimeY = CalcTotalTime(EachY, iterationrest.Cost, 1, iterationrest.Cross);
        if (FR.TotalTime == -1)
          FR.TotalTime = max(TotalTimeX, TotalTimeY);
        else
          FR.TotalTime = min(FR.TotalTime, max(TotalTimeX, TotalTimeY));
        if (TotalTimeX > TotalTimeY)
          break;
      }
    }
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    Iteration iteration = InitIteration(TotalPE, TotalPE);
    int IterationTimes = Ceil(PeriodTimes, 4);
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