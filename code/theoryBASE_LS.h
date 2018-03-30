struct Iteration {
  int PENumb;
  long long UpBound;
  int Round;
  int RunOnCache;
  int RunOnDRAM;
  
  Iteration(int a) {
    PENumb = a;
    UpBound = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    Round = 1;
  }
};

Node NodeList[MAXN];
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;

vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
vector<PEInterval> PEIntervals[MAXPE];

vector<Iteration> IterList;

int DP[MAXN][MAXSIZE + 1];
Node NodeTime[MAXPE][MAXN];

vector<CacheManager> Caches;
vector<CacheBlock> DRAMBlocks;
vector<PEInterval> PETimes;

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
  if (Goods.size() >= MAXN) {
    printf("%lu\n", Goods.size());
  }
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

bool GetStrogePos(int FromId, int FromRound, int ToId, int ToRound) {
  for (int i = 0; i < DRAMBlocks.size(); ++ i) {
    CacheBlock CB = DRAMBlocks[i];
    if (CB.NodeIds.first == FromId && CB.NodeIds.second == ToId 
      && CB.Rounds.first == FromRound && CB.Rounds.second == ToRound) 
      return false;
  }
  return true;
}

void DetectCacheOverflow(Iteration &iteration) {
  for (int i = 1; i <= iteration.PENumb; ++ i) {
    assert(i - 1 >= 0 && i - 1 < Caches.size());
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    // printf("PE:%d/%d\tTimeTrace Size:%lu\n", i, iteration.PENumb, TimeTrace.size());
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      // printf("%d/%lu\n", j, TimeTrace.size() - 1);
      long long ST = TimeTrace[j];
      long long ED = TimeTrace[j + 1];
      vector<CacheBlock> Blocks;
      Caches[i - 1].GetCacheBlockByTime(ST, ED, Blocks);
      if (Blocks.size() == 0)
        continue;

      int PutDRAM = 0;
      for (int k = 0; k < Blocks.size(); ) {
        if (Blocks[k].Memory > CACHESIZE) {
          CacheBlock CB = Blocks[k];
          Blocks.erase(Blocks.begin() + k);
          Caches[i - 1].DeleteCacheBlock(CB);
          DRAMBlocks.push_back(CB);
          PutDRAM = PutDRAM + 1;
        }
        else {
          k++;
        }
      }

      vector<int> Memory;
      for (int k = 0; k < Blocks.size(); ++ k) {
        Memory.push_back(Blocks[k].Memory);
      }
      vector<int> ArrangedSet = ArrangeInFixedSize(Memory, CACHESIZE);
      
      for (int k = ArrangedSet.size() - 1; k >= 0; -- k)
        Blocks.erase(Blocks.begin() + ArrangedSet[k]);

      iteration.RunOnDRAM = iteration.RunOnDRAM + Blocks.size() + PutDRAM;
      iteration.RunOnCache = iteration.RunOnCache - Blocks.size() - PutDRAM;
      for (int k = 0; k < Blocks.size(); ++ k) {
        CacheBlock CB = Blocks[k];
        Caches[i - 1].DeleteCacheBlock(CB);
        DRAMBlocks.push_back(CB);
      }
    }
  }
}

void InitIteration(Iteration &iteration, bool AddCache) {
  sort(NodeList + 1, NodeList + TotalNode + 1, CmpByTopoOrder);
  int NowOrder = -1;
  int Index = 1;
  queue<Node> WaitingQue;
  iteration.Round = 20;
  PETimes.clear();
  for (int i = 1; i <= iteration.PENumb; ++ i) {
    PETimes.push_back(PEInterval(i, 0, 0));
    if (AddCache) {
      CacheManager CM = CacheManager();
      CM.PEId = i;
      Caches.push_back(CM);
    }
  }
  assert(Caches.size() == iteration.PENumb);
  
  do {
    for (; Index <= TotalNode && NodeList[Index].TopoOrder == NowOrder; ++ Index) {
      for (int j = 1; j <= iteration.Round; ++ j) {
        NodeList[Index].Round = j;
        WaitingQue.push(NodeList[Index]);
      }
    }

    int PEIter = 0;
    while (!WaitingQue.empty()) {
      if (PEIter == PETimes.size())
        PEIter = 0;
      Node node = WaitingQue.front();
      WaitingQue.pop();
      long long StartTime = PETimes[PEIter].EndTime;
      int PEId = PETimes[PEIter].PEId;
      
      vector<Edge> Edges = ReEdgeList[node.Id];
      for (int i = 0; i < Edges.size(); ++ i) {
        Edge e = Edges[i];
        long long Cost = Ceil(e.Memory, CACHESPEED);
        if (!GetStrogePos(e.From, node.Round, e.To, node.Round)) {
          if (AddCache) {
            e.Show();
            assert(1 == 0);
          }
          Cost = Ceil(e.Memory, DRAMSPEED);
        }
        StartTime = max(StartTime, NodeTime[node.Round][e.From].EndTime + Cost);
      }

      if (AddCache) {
        iteration.RunOnCache = iteration.RunOnCache + Edges.size();
        for (int i = 0; i < Edges.size(); ++ i) {
          Edge e = Edges[i];
          CacheBlock CB = CacheBlock(e.From, node.Round, e.To, node.Round, e.Memory, NodeTime[node.Round][e.From].EndTime, StartTime);
          Caches[PEId - 1].AddCacheBlock(CB);
        }
      }

      node.PEId = PEId;
      node.SetTime(StartTime, StartTime + node.Cost);
      NodeTime[node.Round][node.Id].Copy(node);

      PETimes[PEIter].EndTime = max(PETimes[PEIter].EndTime, node.EndTime);
      iteration.UpBound = max(iteration.UpBound, node.EndTime);
    }
    if (AddCache) {
      DetectCacheOverflow(iteration);
    }

    if (Index <= TotalNode)
      NowOrder = NodeList[Index].TopoOrder;
  } while (Index <= TotalNode);
}

FinalResult CalcBaseFinalResult(int TotalPE, int PeriodTimes) {
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;

  for (int i = 0; i < IterList.size(); ++ i) {
    printf("Init Iteration:%d\n", i + 1);
    Caches.clear();
    DRAMBlocks.clear();
    InitIteration(IterList[i], true);
    InitIteration(IterList[i], false);
  }

  FinalResult FR = FinalResult();
  if (IterList.size() == 1) {
    int Launches = Ceil(TotalPE, IterList[0].PENumb);
    int X = Ceil(Ceil(PeriodTimes, Launches), IterList[0].Round);
    FR.TotalTime = X * IterList[0].UpBound;
    FR.Prelogue = 0;
    FR.Retiming = 0;
    FR.RunOnCache = IterList[0].RunOnCache * X * Launches;
    FR.RunOnDRAM = IterList[0].RunOnDRAM * X * Launches;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    int Launches = TotalPE / IterList[0].PENumb;
    for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
      int EachY = PeriodTimes - EachX;
      int X = Ceil(Ceil(EachX, Launches), IterList[0].Round);
      int Y = Ceil(EachY, IterList[1].Round);
      long long TotalTimeX = IterList[0].UpBound * X;
      long long TotalTimeY = IterList[1].UpBound * Y;
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
  FinalResult FR = CalcBaseFinalResult(TotalPE, PeriodTimes);
  return FR;
}