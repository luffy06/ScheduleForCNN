struct Iteration {
  int PENumb;
  long long UpBound;
  int Round;
  int RunOnCache;
  int RunOnDRAM;
  double Ratio;
  
  Iteration(int a) {
    PENumb = a;
    UpBound = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    Round = 1;
    Ratio = 0.0;
  }
};

vector<PEInterval> PEIntervals[MAXPE];

vector<Iteration> IterList;

Node NodeTime[MAXPE][MAXN];

vector<PEInterval> PETimes;

bool CmpByTopoOrder(Node a, Node b) {
  if (a.TopoOrder != b.TopoOrder)
    return a.TopoOrder < b.TopoOrder;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
}

void Init(int TotalPE, int UpRound) {
  memset(Degree, 0, sizeof(Degree));

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  int NeedPE = GetTopology(TotalNode);
  // printf("Multi:%d\n", NeedPE);

  if (TotalPE >= NeedPE) {
    IterList.push_back(Iteration(NeedPE));
    if (TotalPE % NeedPE != 0)
      IterList.push_back(Iteration(TotalPE % NeedPE));
  }
  else {
    IterList.push_back(Iteration(TotalPE));
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

void DetectCacheOverflow(Iteration &iteration) {
  for (int i = 1; i <= iteration.PENumb; ++ i) {
    assert(i - 1 >= 0 && i - 1 < Caches.size());
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    // printf("PE:%d/%d\tTimeTrace Size:%lu\n", i, iteration.PENumb, TimeTrace.size());
    int Index = 0;
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      // printf("%d/%lu\n", j, TimeTrace.size() - 1);
      long long ST = TimeTrace[j];
      long long ED = TimeTrace[j + 1];
      vector<CacheBlock> Blocks;
      Index = Caches[i - 1].GetCacheBlockByTime(ST, ED, Blocks, Index);
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
  iteration.Round = 5;
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
    for (; Index <= TotalNode && NodeList[Index].Layer == NowOrder; ++ Index) {
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
      NowOrder = NodeList[Index].Layer;
  } while (Index <= TotalNode);
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + 5LL * NodeList[i].Cost;
  double Ratio = (TotalCost * 1.0) / (iteration.UpBound * iteration.PENumb);
  iteration.Ratio = Ratio;
  // printf("Iteration Ratio:%.6f\n", Ratio);
}

FinalResult CalcBaseFinalResult(int TotalPE, int PeriodTimes) {
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;

  for (int i = 0; i < IterList.size(); ++ i) {
    // printf("Init Iteration:%d\n", i + 1);
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
    FR.MAXRatio = IterList[0].Ratio;
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
    FR.MAXRatio = (IterList[1].Ratio + IterList[0].Ratio) / 2;
  }
  return FR;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);
  FinalResult FR = CalcBaseFinalResult(TotalPE, PeriodTimes);
  return FR;
}