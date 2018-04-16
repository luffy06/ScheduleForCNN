const int REPEATLIMITED = 505;
int REPEAT = 2;

// TopoOrder:         small -> big
// MaxOutEdge - Cost: big -> small
// Id:                small -> big
struct NodeComparationByOutEdge {
  bool operator() (const Node &a, const Node &b) const {
    if (a.TopoOrder != b.TopoOrder)
      return a.TopoOrder > b.TopoOrder;
    else if (a.MaxOutEdge - a.Cost != b.MaxOutEdge - b.Cost)
      return a.MaxOutEdge - a.Cost < b.MaxOutEdge - b.Cost;
    return a.Id > b.Id;
  }
};

// TopoOrder: small -> big
// Cost:      big -> small
// Id:        small -> big
struct NodeComparationByCost {
  bool operator() (const Node &a, const Node &b) const {
    if (a.TopoOrder != b.TopoOrder)
      return a.TopoOrder > b.TopoOrder;
    else if (a.Cost != b.Cost)
      return a.Cost < b.Cost;
    return a.Id > b.Id;
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
  double MaxRatio;

  Iteration(int a, int b, int c) {
    UpBound = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    PENumb = a;
    PhasePE = b;
    TotalNode = c;
    MaxRatio = 0;
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
    Retiming = MaxRetiming - MinRetiming;
    Prelogue = Retiming * UpBound;
  }
};

Node NodeTime[REPEATLIMITED + 1][MAXN];
Node IterNodeTime[REPEATLIMITED * 2 + 1][MAXN];

vector<Iteration> IterList;

bool ReChecked[MAXN][REPEATLIMITED * 2 + 1];

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

int Init(int TotalPE) {
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
  // printf("Min:%d\tMAX:%d\tREPEAT:%d\tAverage Repeat:%d\n", MinRepeat, MaxRepeat, 
  //                     REPEAT, (RepeatCount == 0 ? 0 : SumRepeat / RepeatCount));

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

int CalculateFromNodeRetiming(long long FE, long long P, long long E, long long TS, long long TR) {
  // FE + Retiming * P + E <= TS + TR * P
  int Retiming = Floor(TS + TR * P - FE - E, P);
  return Retiming;
}

int CalculateToNodeRetiming(long long FE, long long FR, long long P, long long E, long long TS) {
  // FE + FR * P + E <= TS + Retiming * P
  int Retiming = Ceil(FE + FR * P - TS + E, P);
  return Retiming;
}

long long GetStartTime(long long StartTime, int NodeId, int Round) {
  vector<Edge> Edges = ReEdgeList[NodeId];
  for (int i = 0; i < Edges.size(); i++) {
    Edge e = Edges[i];
    if (e.Memory > CACHESIZE)
      continue;
    long long NStartTime = NodeTime[Round][e.From].EndTime + Ceil(e.Memory, CACHESPEED);
    StartTime = max(StartTime, NStartTime);
  }
  return StartTime;
}

void InitPhaseOrigin(Phase &phase) {
  vector<TimeInterval> IntervalQue;
  priority_queue<Node, vector<Node>, NodeComparationByOutEdge> NodeQue;
  int PENumb = phase.PENumb;
  for (int i = 1; i <= PENumb; ++ i) {
    TimeInterval Interval = TimeInterval(i, 0, 0);
    IntervalQue.push_back(Interval);
  }
  assert(PENumb > 0);
  int Index = 1;
  int NowOrder = -1;
  do {
    for (; Index <= TotalNode && NodeList[Index].TopoOrder == NowOrder; ++ Index)
      NodeQue.push(NodeList[Index]);
    int PEIndex = 0;
    while (!NodeQue.empty()) {
      if (PEIndex == IntervalQue.size())
        PEIndex = 0;
      bool Horizontal = NodeQue.size() >= PENumb;
      if (!Horizontal) {
        Node node = NodeQue.top();
        NodeQue.pop();
        for (int k = 1; k <= REPEAT; ++ k, ++ PEIndex) {
          if (PEIndex == IntervalQue.size())
            PEIndex = 0;
          long long StartTime = GetStartTime(IntervalQue[PEIndex].EndTime, node.Id, k);
          node.PEId = IntervalQue[PEIndex].PEId;
          node.Round = k;
          node.SetTime(StartTime, StartTime + node.Cost);
          NodeTime[node.Round][node.Id].Copy(node);
          IntervalQue[PEIndex].SetTime(IntervalQue[PEIndex].StartTime, node.EndTime);
        }
      }
      else {
        for (; PEIndex < IntervalQue.size(); ++ PEIndex) {
          Node node = NodeQue.top();
          NodeQue.pop();
          for (int k = 1; k <= REPEAT; ++ k) {
            long long StartTime = GetStartTime(IntervalQue[PEIndex].EndTime, node.Id, k);
            if (k > 1)
              StartTime = max(StartTime, NodeTime[k - 1][node.Id].EndTime);
            node.PEId = IntervalQue[PEIndex].PEId;
            node.Round = k;
            node.SetTime(StartTime, StartTime + node.Cost);
            NodeTime[node.Round][node.Id].Copy(node);
            IntervalQue[PEIndex].SetTime(IntervalQue[PEIndex].StartTime, node.EndTime);
          }
        }
      }
    }
    if (Index <= TotalNode)
      NowOrder = NodeList[Index].TopoOrder;
  } while (Index <= TotalNode);
  phase.PETimes.push_back(PEInterval(0, -1, -1));
  for (int i = 0; i < IntervalQue.size(); ++ i) {
    TimeInterval TI = IntervalQue[i];
    phase.PETimes.push_back(PEInterval(TI.PEId, TI.StartTime, TI.EndTime));
  }
  phase.SortPETimes();

  long long MaxEndtime = 0;
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= REPEAT; ++ j)
      TotalCost = TotalCost + NodeTime[j][i].Cost;
  for (int i = 1; i <= PENumb; ++ i)
    MaxEndtime = max(MaxEndtime, phase.PETimes[i].EndTime);
  phase.Ratio = TotalCost / (1.0 * MaxEndtime * PENumb);
  // printf("Ratio of Phase:%.6f\n", phase.Ratio);
}

void InitPhasePriority(Phase &phase) {
  priority_queue<TimeInterval> IntervalQue;
  priority_queue<Node, vector<Node>, NodeComparationByOutEdge> NodeQue;
  int PENumb = phase.PENumb;
  phase.PETimes.clear();
  for (int i = 1; i <= PENumb; ++ i) {
    TimeInterval Interval = TimeInterval(i, 0, 0);
    IntervalQue.push(Interval);
  }
  assert(PENumb > 0);
  int Index = 1;
  int NowLayer = -1;
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
    for (; Index <= TotalNode && NodeList[Index].Layer == NowLayer; ++ Index)
      NodeQue.push(NodeList[Index]);
    int PEIndex = 0;
    while (!NodeQue.empty()) {
      Node node = NodeQue.top();
      NodeQue.pop();
      for (int k = 1; k <= REPEAT; ++ k) {
        TimeInterval TI = IntervalQue.top();
        IntervalQue.pop();
        node.Retiming = 0;
        node.Round = k;
        node.PEId = TI.PEId;
        node.SetTime(TI.EndTime, TI.EndTime + node.Cost);
        NodeTime[node.Round][node.Id].Copy(node);
        TI.SetTime(TI.StartTime, node.EndTime);
        MaxEndtime = max(MaxEndtime, node.EndTime);
        IntervalQue.push(TI);
      }
    }
    if (Index <= TotalNode)
      NowLayer = NodeList[Index].Layer;
  } while (Index <= TotalNode);
  phase.PETimes.push_back(PEInterval(0, -1, -1));
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
  // printf("Ratio of Phase:%.6f\n", phase.Ratio);
}

void PutEdgeIntoCache(int PENumb, int UpBound) {
  for (int i = 0; i < PENumb; ++ i) {
    CacheManager CM = CacheManager();
    CM.PEId = i + 1;
    Caches.push_back(CM);
  }

  for (int i = 1; i <= TotalNode; ++ i) {
    int FromId = NodeList[i].Id;
    vector<Edge> Edges = EdgeList[FromId];
    for (int j = 1; j <= REPEAT * 2; ++ j) {
     Node FromNode = IterNodeTime[j][FromId]; 
      for (int k = 0; k < Edges.size(); ++ k) {
        Edge e = Edges[k];
        Node ToNode = IterNodeTime[j][e.To];
        if (e.Memory <= CACHESIZE && FromNode.EndTime + FromNode.Retiming * UpBound 
              + Ceil(e.Memory, CACHESPEED) >= ToNode.StartTime + ToNode.Retiming 
              * UpBound) {
          int Retiming = CalculateToNodeRetiming(FromNode.EndTime, FromNode.Retiming, 
                          UpBound, Ceil(e.Memory, CACHESPEED), ToNode.StartTime);
          ToNode.Retiming = max(ToNode.Retiming, Retiming);
          IterNodeTime[j][e.To].Copy(ToNode);
          assert(FromNode.EndTime + FromNode.Retiming * UpBound < ToNode.StartTime + ToNode.Retiming * UpBound);
        }
      }
    }
  }

  for (int i = 1; i <= TotalNode; ++ i) {
    vector<Edge> Edges = EdgeList[i];
    for (int j = 1; j <= REPEAT * 2; ++ j) {
      Node FromNode = IterNodeTime[j][i];
      for (int k = 0; k < Edges.size(); ++ k) {
        Edge e = Edges[k];
        Node ToNode = IterNodeTime[j][e.To];
        if (e.Memory <= CACHESIZE) {
          CacheBlock CB = CacheBlock(FromNode.Id, FromNode.Round, ToNode.Id, 
                        ToNode.Round, e.Memory, FromNode.EndTime + FromNode.Retiming 
                        * UpBound, ToNode.StartTime + ToNode.Retiming * UpBound);
          Caches[ToNode.PEId - 1].AddCacheBlock(CB);          
        }
        else {
          CacheBlock CB = CacheBlock(FromNode.Id, FromNode.Round, ToNode.Id, 
                        ToNode.Round, e.Memory, 0, 1);
          DRAMBlocks.push_back(CB);
          ReChecked[FromNode.Id][FromNode.Round] = true;
        }
      }
    }
  }

}

void DetectCacheOverflow(Iteration &iteration) {
  for (int i = 1; i <= iteration.PENumb; ++ i) {
    assert(i - 1 >= 0 && i - 1 < Caches.size());
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    // printf("PE:%d/%d\tTimeTrace Size:%lu\n", i, iteration.PENumb, TimeTrace.size());
    int Index = 0;
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      long long ST = TimeTrace[j];
      long long ED = TimeTrace[j + 1];
      vector<CacheBlock> Blocks;
      long long MemorySum = 0;
      Index = Caches[i - 1].GetCacheBlockByTime(ST, ED, Blocks, MemorySum, Index);
      if (MemorySum <= CACHESIZE)
        continue;

      vector<int> Memory;
      for (int k = 0; k < Blocks.size(); ++ k)
        Memory.push_back(Blocks[k].Memory);
      set<int> ArrangedSet = ArrangeInFixedSize(Memory, CACHESIZE);
      
      iteration.RunOnDRAM = iteration.RunOnDRAM + Blocks.size();
      iteration.RunOnCache = iteration.RunOnCache - Blocks.size();
      for (int k = 0; k < Blocks.size(); ++ k) {
        if (ArrangedSet.find(k) != ArrangedSet.end())
          continue;
        CacheBlock CB = Blocks[k];
        Caches[i - 1].DeleteCacheBlock(CB);
        DRAMBlocks.push_back(CB);
        ReChecked[CB.NodeIds.first][CB.Rounds.first] = true;
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

void BFS(Node KeyNode, Iteration &iteration) {
  // printf("BFS\n");
  long long PeriodTime = iteration.UpBound;
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  q.push(KeyNode);
  while (!q.empty()) {
    Node FromNode = q.top();
    q.pop();
    // FromNode.Show();
    ReChecked[FromNode.Id][FromNode.Round] = false;

    vector<Edge> Edges = EdgeList[FromNode.Id];
    for (int i = 0; i < Edges.size(); ++ i) {
      Edge e = Edges[i];
      Node ToNode = IterNodeTime[FromNode.Round][Edges[i].To];
      long long Cost = (GetStrogePos(FromNode.Id, FromNode.Round, ToNode.Id, ToNode.Round) 
                      ? Ceil(e.Memory, CACHESPEED) : Ceil(e.Memory, DRAMSPEED));
      if (FromNode.EndTime + FromNode.Retiming * PeriodTime + Cost > ToNode.StartTime + ToNode.Retiming * PeriodTime) {
        ToNode.Retiming = CalculateToNodeRetiming(FromNode.EndTime, FromNode.Retiming, PeriodTime, Cost, ToNode.StartTime);
        IterNodeTime[ToNode.Round][ToNode.Id].Copy(ToNode);
        q.push(ToNode);
      }
    }
  }
}

void InitIteration(Iteration &iteration) {
  int PhasePE = iteration.PhasePE;
  Phase phase = Phase(PhasePE, TotalNode);
  // printf("Init Phase\n");
  for (REPEAT = 2; REPEAT <= REPEATLIMITED; ++ REPEAT) {
    InitPhasePriority(phase);
    // InitPhaseOrigin(phase);
    if (phase.Ratio >= LIMITEDRATIO)
      break;
  }
  // printf("Init Iteration\n");

  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= REPEAT; ++ j)
      IterNodeTime[j][i] = NodeTime[j][i];

  int Shift = iteration.PENumb - phase.PENumb;
  // printf("Shift:%d\n", Shift);
  long long MaxEndtime = 0;
  vector<int> Moves;
  Moves.push_back(0);
  // printf("Calc Shift Out Part\n");
  for (int i = 1; i <= Shift; ++ i) {
    long long Offset = 0;
    Moves.push_back(Offset);
    iteration.UpBound = max(iteration.UpBound, phase.PETimes[i].EndTime + Offset);    
  }

  // printf("Calc Shift In Part\n");
  for (int i = Shift + 1; i <= phase.PENumb; ++ i) {
    int j = phase.PENumb + Shift + 1 - i;
    int Offset = phase.PETimes[j].EndTime - phase.PETimes[i].StartTime;
    Offset = max(Offset, Moves[Moves.size() - 1]);
    Moves.push_back(Offset);
    iteration.UpBound = max(iteration.UpBound, phase.PETimes[i].EndTime + Offset);    
  }

  // printf("Calc Second Phase\n");
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 1; j <= REPEAT; ++ j) {
      Node node = NodeTime[j][i];
      TotalCost = TotalCost + node.Cost * 2;
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

  // printf("Put Edge Into Cache\n");
  PutEdgeIntoCache(iteration.PENumb, iteration.UpBound);

  double Ratio = TotalCost / (1.0 * MaxEndtime * iteration.PENumb);
  iteration.MaxRatio = Ratio;
  // printf("Ratio of Iteration:%.6f\n", Ratio);
  // printf("UpBound:%lld\n", iteration.UpBound);

  assert(iteration.UpBound > 0);
  for (int i = 0; i < Caches.size(); ++ i)
    iteration.RunOnCache = iteration.RunOnCache + Caches[i].Cache.size();

  // printf("Detect Cache Overflow\n");
  DetectCacheOverflow(iteration);
  // printf("DRAMBlocks:%lu\n", DRAMBlocks.size());


  vector<Node> ReCheckedNodes;
  for (int i = 1; i <= TotalNode; ++ i) 
    for (int j = 1; j <= REPEAT * 2; ++ j) 
      if (ReChecked[i][j]) ReCheckedNodes.push_back(IterNodeTime[j][i]);
  
  sort(ReCheckedNodes.begin(), ReCheckedNodes.end(), CmpByTopoOrder);
  // printf("ReChecked Nodes\n");
  for (int i = ReCheckedNodes.size() - 1; i >= 0; -- i) {
    Node KeyNode = ReCheckedNodes[i];
    if (ReChecked[KeyNode.Id][KeyNode.Round])
      BFS(IterNodeTime[KeyNode.Round][KeyNode.Id], iteration);
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
    memset(ReChecked, false, sizeof(ReChecked));
    InitIteration(IterList[i]);
  }

  if (IterList.size() == 1) {
    Iteration iteration = IterList[0];
    int Launches = Ceil(TotalPE, NeedPE);
    int X = Ceil(Ceil(PeriodTimes, Launches), 2 * REPEAT);
    FR.TotalTime = iteration.Prelogue + 1LL * max(0, X - 1) * iteration.UpBound;
    FR.Kernel = iteration.UpBound;
    FR.Prelogue = iteration.Prelogue;
    FR.Retiming = iteration.Retiming;
    FR.RunOnCache = X * iteration.RunOnCache * Launches;
    FR.RunOnDRAM = X * iteration.RunOnDRAM * Launches;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
    FR.MAXRatio = IterList[0].MaxRatio;
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
        FR.Kernel = (iterationX.UpBound + iterationY.UpBound) / 2.0;
        FR.RunOnCache = iterationX.RunOnCache * X * Launches + iterationY.RunOnCache * Y;
        FR.RunOnDRAM = iterationX.RunOnDRAM * X * Launches + iterationY.RunOnDRAM * Y;
      }
    }
    FR.Retiming = iterationX.Retiming;
    FR.Prelogue = iterationX.Prelogue;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
    FR.MAXRatio = (IterList[0].MaxRatio + IterList[1].MaxRatio) / 2;
  }
  return FR;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  int NeedPE = Init(TotalPE);
  FinalResult FR = CalcResult(TotalPE, NeedPE, PeriodTimes);
  return FR;
}