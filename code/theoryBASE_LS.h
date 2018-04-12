Node NodeTime[MAXR][MAXN];

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

TwoInt DetectCacheOverflow(int PENumb, int RunOnCache) {
  TwoInt RunPlace = make_pair(RunOnCache, 0);
  for (int i = 1; i <= PENumb; ++ i) {
    assert(i - 1 >= 0 && i - 1 < Caches.size());
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    int Index = 0;
    // printf("PE:%d/%d\tTimeTrace Size:%lu\n", i, PENumb, TimeTrace.size());
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      // if (j % 10000 == 0)
      //   printf("%d/%lu %d\n", j, TimeTrace.size(), Index);
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
      
      RunPlace.first = RunPlace.first - Blocks.size();
      RunPlace.second = RunPlace.second + Blocks.size();
      for (int k = 0; k < Blocks.size(); ++ k) {
        if (ArrangedSet.find(k) != ArrangedSet.end())
          continue;
        CacheBlock CB = Blocks[k];
        Caches[i -  1].DeleteCacheBlock(CB);
        NodeTime[CB.NodeIds.first][CB.Rounds.first].Certained = false;
      }
    }
  }
  return RunPlace;
}

void BFS() {
  vector<Node> ReCheckedNodes;
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 1; j <= PeriodTimes; ++ j) {
      if (!NodeTime[j][i].Certained) {
        ReCheckedNodes.push_back(NodeTime[j][i]);
      }
    }
  }

  sort(ReCheckedNodes.begin(), ReCheckedNodes.end(), CmpByTopoOrder);

  for (int i = 0; i < ReCheckedNodes.size(); ++ i) {
    Node node = ReCheckedNodes[i];
    if (NodeTime[node.Round][node.Id].Certained)
      continue;
    priority_queue<Node, vector<Node>, NodeComparationByCost> que;
    que.push(node);
    while (!que.empty()) {
      Node FromNode = que.top();
      que.pop();
      NodeTime[FromNode.Round][FromNode.Id].Certained = true;

      vector<Edge> Edges = EdgeList[FromNode.Id];
      for (int j = 0; j < Edges.size(); ++ j) {
        Edge e = Edges[j];
        long long StartTime = FromNode.EndTime + Ceil(e.Memory, DRAMSPEED);
        long long EndTime = StartTime + NodeTime[FromNode.Round][e.To].Cost;
        if (NodeTime[FromNode.Round][e.To].StartTime < StartTime) {
          NodeTime[FromNode.Round][e.To].SetTime(StartTime, EndTime);
          que.push(NodeTime[FromNode.Round][e.To]);
        }
      }
    }
  }
}

int Init() {
  GetTopology();

  int RunOnCache = 0;
  priority_queue<TimeInterval> PEs;
  for (int i = 1; i <= TotalPE; ++ i) {
    PEs.push(TimeInterval(i, 0, 0));
    CacheManager CM = CacheManager();
    CM.PEId = i;
    Caches.push_back(CM);
  }

  sort(NodeList + 1, NodeList + 1 + TotalNode, CmpByLayer);
  for (int r = 1; r <= PeriodTimes; ++ r) {
    for (int i = 1; i <= TotalNode; ++ i) {
      TimeInterval TI = PEs.top();
      PEs.pop();

      long long StartTime = TI.EndTime;
      vector<Edge> Edges = ReEdgeList[i];
      for (int j = 0; j < Edges.size(); ++ j) {
        Edge e = Edges[j];
        Node FromNode = NodeTime[r][e.From];
        StartTime = max(StartTime, FromNode.EndTime + Ceil(e.Memory, CACHESPEED));
      }

      for (int j = 0; j < Edges.size(); ++ j) {
        Edge e = Edges[j];
        Node FromNode = NodeTime[r][e.From];
        long long Cost = Ceil(e.Memory, CACHESPEED);
        CacheBlock CB = CacheBlock(e.From, r, e.To, r, e.Memory, FromNode.EndTime, 
                                  StartTime + Cost);
        Caches[TI.PEId - 1].AddCacheBlock(CB);
        RunOnCache = RunOnCache + 1;
      }

      int Id = NodeList[i].Id;
      NodeTime[r][Id].Copy(NodeList[i]);
      NodeTime[r][Id].Certained = true;
      NodeTime[r][Id].PENumb = TI.Count;
      NodeTime[r][Id].PEId = TI.PEId;
      NodeTime[r][Id].Round = r;
      NodeTime[r][Id].SetTime(StartTime, StartTime +  NodeTime[r][Id].Cost);

      TI.EndTime = max(TI.EndTime, NodeTime[r][Id].EndTime);
      TI.Count = TI.Count + 1;
      PEs.push(TI);
    }
  }
  return RunOnCache;
}

FinalResult CalcBaseFinalResult(int RunOnCache) {
  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;

  printf("Detect Cache Overflow\n");
  TwoInt RunPlace = DetectCacheOverflow(TotalPE, RunOnCache);
  printf("BFS\n");
  BFS();

  vector<Node> PELine[MAXPE];
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 1; j <= PeriodTimes; ++ j) {
      int PEId = NodeTime[j][i].PEId;
      PELine[PEId].push_back(NodeTime[j][i]);
    }
  }

  long long TotalTime = 0;
  for (int i = 1; i <= TotalPE; ++ i) {
    sort(PELine[i].begin(), PELine[i].end(), CmpByPENumb);
    TotalTime = max(TotalTime, PELine[i][0].EndTime);
    for (int j = 1; j < PELine[i].size(); ++ j) {
      assert(PELine[i][j].PENumb > PELine[i][j - 1].PENumb);
      Node PreNode = PELine[i][j - 1];
      Node NowNode = PELine[i][j];
      if (PreNode.EndTime > NowNode.StartTime) {
        NowNode.SetTime(PreNode.EndTime, PreNode.EndTime + NowNode.Cost);
        NodeTime[NowNode.Round][NowNode.Id].Copy(NowNode);
      }
      TotalTime = max(TotalTime, NowNode.EndTime);
    }
  }

  FinalResult FR = FinalResult();
  FR.TotalTime = TotalTime;
  FR.Prelogue = 0;
  FR.Retiming = 0;
  FR.RunOnCache = RunPlace.first;
  FR.RunOnDRAM = RunPlace.second;
  FR.MAXRatio = (1.0 * TotalCost * PeriodTimes) / (1.0 * TotalTime * TotalPE);
  FR.CPURatio = FR.MAXRatio;

  return FR;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  int RunOnCache = Init();
  FinalResult FR = CalcBaseFinalResult(RunOnCache);
  return FR;
}