
// TopoOrder: big -> small
// Cost:      big -> small
// Id:        small -> big
struct NodeComparationByCost {
  bool operator() (const Node &a, const Node &b) const {
    if (a.TopoOrder != b.TopoOrder)
      return a.TopoOrder < b.TopoOrder;
    else if (a.Cost != b.Cost)
      return a.Cost < b.Cost;
    return a.Id > b.Id;
  }
};

struct NodeGenerator {
  int TotalNode;
  int NeedPE;
  int Div;
  int UpBound;
  int UpRound;
  long long Prelogue;
  int Retiming;
  long long RunOnCache;
  long long RunOnDRAM;
  double CacheRatio;
  double MaxRatio;
  vector<Node> StartTable;
  
  int AllRound;
  int TotalRound;
  long long TotalTime;

  NodeGenerator() {
    TotalNode = 0;
    NeedPE = 0;
    UpBound = 0;
    Prelogue = -1;
    RunOnCache = RunOnDRAM = 0;
    MaxRatio = CacheRatio = 0;
    TotalRound = 0;
    TotalTime = 0;
    StartTable.clear();
  }

  NodeGenerator(int a, int b, int c, int MaxRound, Node NodeList[MAXN]) {
    TotalNode = a;
    NeedPE = b;
    Div = c;
    UpBound = 0;
    Prelogue = -1;
    RunOnCache = RunOnDRAM = 0;
    MaxRatio = CacheRatio = 0;
    StartTable.clear();
    TotalRound = 0;
    TotalTime = 0;
    CalcBound(MaxRound, NodeList);
  }

  double Init(Node NodeList[MAXN]) {
    priority_queue<Node, vector<Node>, NodeComparationByEndTime> q;
    StartTable.clear();
    for (int i = 1; i <= NeedPE; ++ i) {
      Node n = Node(0, 0);
      n.PEId = i;
      n.SetTime(0, 0);
      q.push(n);
    }
    sort(NodeList + 1, NodeList + TotalNode + 1, CmpByCost);
    for (int i = 1; i <= TotalNode; ++ i) {
      for (int j = 1; j <= UpRound; ++ j) {
        Node Emp = q.top();
        q.pop();

        Node n = Node();
        n.Copy(NodeList[i]);
        n.Round = j;
        n.PEId = Emp.PEId;
        n.SetTime(Emp.EndTime, Emp.EndTime + n.Cost);
        q.push(n);
        UpBound = max(UpBound, (int)(Emp.EndTime + n.Cost));
        StartTable.push_back(n);
      }
    }
    sort(NodeList + 1, NodeList + TotalNode + 1, CmpById);
    // calculate the use ratio of cpu
    assert(UpBound != 0);
    double Down = UpBound * NeedPE;
    double Up = 0;
    for (int i = 1; i <= TotalNode; ++ i)
      Up = Up + NodeList[i].Cost;
    Up = Up * UpRound;
    double Ratio = Up / Down;
    return Ratio;
  }

  void CalcBound(int MaxRound, Node NodeList[MAXN]) {
    int TargetRound = 1;
    for (UpRound = 1; UpRound <= MaxRound; ++ UpRound) {
      double NowRatio = Init(NodeList);
      if (NowRatio >= LIMITEDRATIO) {
        TargetRound = UpRound;
        break;
      }
      else if (NowRatio > MaxRatio) {
        TargetRound = UpRound;
        MaxRatio = NowRatio;
      }
    }
    UpRound = TargetRound;
    MaxRatio = Init(NodeList);
    // printf("MaxRatio:%.6f\n", MaxRatio);
    for (int i = 0; i < StartTable.size(); ++ i) {
      StartTable[i].SetTime(0, UpBound);
      // StartTable[i].Certained = true;
    }
    sort(StartTable.begin(), StartTable.end(), CmpById);
  }

  void SetNode(Node ArNode) {
    bool Found = false;
    TwoInt Int = BinarySearch(ArNode.Id);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      if (StartTable[i].Round == ArNode.Round) {
        if (ArNode.StartTime < 0 || ArNode.EndTime > UpBound) {
          printf("### Bad Time ###\n");
          ArNode.Show();
        }
        assert(ArNode.StartTime >= 0 && ArNode.EndTime <= UpBound);
        StartTable[i].SetTime(ArNode.StartTime, ArNode.EndTime);
        StartTable[i].Retiming = ArNode.Retiming;
        StartTable[i].Certained = ArNode.Certained;
        Found = true;
        break;
      }
    }
    if (!Found) {
      printf("### Cannot Found Node ###\n");
      ArNode.Show();
    }
    assert(Found == true);
  }

  Node GetNode(int NodeId, int Round) {
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i)
      if (StartTable[i].Round == Round)
        return StartTable[i];
    printf("### Not Found Node:Id:%d\tRound:%d ###\n", NodeId, Round);
    assert(1 == 0);
    return Node();
  }

  TwoInt BinarySearch(int NodeId) {
    int L = 0;
    int R = StartTable.size() - 1;
    if (StartTable[L].Id == NodeId) {
      R = L + UpRound;
    }
    else {
      while (R - L > 1) {
        int M = (L + R) >> 1;
        if (StartTable[M].Id < NodeId)
          L = M;
        else
          R = M;
      }
      L = R;
      R = L + UpRound;
    }
    return make_pair(L, R);
  }

  Node GetChoosedNode(int NodeId, int Round) {
    vector<Node> ChoosedNodes;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      assert (StartTable[i].Id == NodeId);
      if (StartTable[i].Round == Round)
        return StartTable[i];
    }
    assert(1 == 0);
    return Node();
  }

  vector<Node> GetSamePEOtherNodes(int PEId, int NodeId, int Round, int StartTime, int EndTime) {
    vector<Node> Nodes;
    for (int i = 0; i < StartTable.size(); ++ i) {
      if (StartTable[i].PEId != PEId)
        continue;
      if (!StartTable[i].Certained && StartTable[i].StartTime >= StartTime && StartTable[i].EndTime <= EndTime)
        Nodes.push_back(StartTable[i]);
    }
    for (int i = 0; i < Nodes.size(); ++ i) {
      if (Nodes[i].Id == NodeId && Nodes[i].Round == Round) {
        Nodes.erase(Nodes.begin() + i);
        break;
      }
    }
    return Nodes;
  }

  void CalcPrelogue() {
    int MinRetiming = INF;
    int MaxRetiming = -1;
    for (int i = 0; i < StartTable.size(); ++ i) {
      MinRetiming = min(MinRetiming, StartTable[i].Retiming);
      MaxRetiming = max(MaxRetiming, StartTable[i].Retiming);
    }
    Retiming = MaxRetiming - MinRetiming;
    Prelogue = Retiming * UpBound;
    TotalTime = Prelogue - UpBound;
  }

  void PlaceOnePeriod() {
    TotalRound = TotalRound + UpRound;
    TotalTime = TotalTime + UpBound;
  }

  void Show() {
    sort(StartTable.begin(), StartTable.end(), CmpByPE);
    int LastPEId = -1;
    int LastEndTime = 0;
    printf("Arrangement");
    for (int i = 0; i < StartTable.size(); ++ i) {
      if (StartTable[i].PEId != LastPEId) {
        if (LastEndTime != UpBound && LastPEId != -1) {
          for (int j = LastEndTime; j < UpBound; ++ j)
            printf("-");
        }
        printf("\n");
        LastPEId = StartTable[i].PEId;
        LastEndTime = 0;
      }
      if (!StartTable[i].Certained)
        continue;
      if (LastEndTime < StartTable[i].StartTime)
        for (int j = LastEndTime; j < StartTable[i].StartTime; ++ j)
          printf("-");
      char c =  'A' + StartTable[i].Id - 1;
      for (int j = 0; j < StartTable[i].Cost; ++ j)
        printf("%c%d", c, StartTable[i].Round);
      LastEndTime = StartTable[i].EndTime;
    }
    if (LastEndTime != UpBound && LastPEId != -1) {
      for (int j = LastEndTime; j < UpBound; ++ j)
        printf("-");
    }
    printf("\n\n");
    sort(StartTable.begin(), StartTable.end(), CmpById);
  }

  void ShowEach(bool OnlyUncertained) {
    for (int i = 0; i < StartTable.size(); ++ i) {
      if ((OnlyUncertained && !StartTable[i].Certained) || !OnlyUncertained)
        StartTable[i].Show();
    }
  }

};

struct NodeGeneratorComparator {
  bool operator()(NodeGenerator &a, NodeGenerator &b) const {
    if (a.TotalTime != b.TotalTime)
      return a.TotalTime + a.UpBound > b.TotalTime + b.UpBound;
    return a.UpBound / a.UpRound > b.UpBound / b.UpRound;
  }
};


vector<NodeGenerator> NgList;

vector<PEInterval> PEIntervals[MAXPE];

bool Checked[MAXN][MINR];
bool ReChecked[MAXN][MINR];

void ShowInterval(int PEId) {
  printf("PEId:%d\n", PEId);
  if (PEIntervals[PEId].size() == 0) {
    printf("None Interval\n");
    return;
  }
  for (int i = 0; i < PEIntervals[PEId].size(); ++ i) {
    printf("[%lld, %lld]\n", PEIntervals[PEId][i].StartTime, PEIntervals[PEId][i].EndTime);
  }
}

void Init(int TotalPE, int UpRound) {
  memset(Degree, 0, sizeof(Degree));
  memset(Checked, false, sizeof(Checked));
  memset(ReChecked, false, sizeof(ReChecked));

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  int NeedPE = GetTopology();
  // printf("Multi:%d\n", NeedPE);

  for (int i = 1; i <= TotalPE; ++ i)
    NgList.push_back(NodeGenerator(TotalNode, i, TotalPE / i, UpRound, NodeList));
}

vector<Node> GetKeyNodeSet(vector<Node> &ChoosedNodes) {
  vector<Node> KeyNodeSet;
  for (int i = 0; i < ChoosedNodes.size();) {
    if (Checked[ChoosedNodes[i].Id][ChoosedNodes[i].Round])
      ChoosedNodes.erase(ChoosedNodes.begin() + i);
    else
      ++ i;
  }
  long long MaxCost = -1;
  for (int i = 0; i < ChoosedNodes.size(); ++ i)
    MaxCost = max(MaxCost, ChoosedNodes[i].Cost);

  for (int i = 0; i < ChoosedNodes.size(); ) {
    if (ChoosedNodes[i].Cost >= (int)(MaxCost * ALPHA)) {
      KeyNodeSet.push_back(ChoosedNodes[i]);
      ChoosedNodes.erase(ChoosedNodes.begin() + i);
    }
    else {
      ++ i;
    }
  }

  sort(KeyNodeSet.begin(), KeyNodeSet.end(), CmpByCost);
  return KeyNodeSet;
}

void DeleteInterval(int PEId, int Int, int StartTime, int EndTime) {
  assert(Int >= 0 && Int < PEIntervals[PEId].size());
  if (PEIntervals[PEId][Int].StartTime < StartTime)
    PEIntervals[PEId].push_back(PEInterval(PEId, PEIntervals[PEId][Int].StartTime, StartTime));
  if (EndTime < PEIntervals[PEId][Int].EndTime)
    PEIntervals[PEId].push_back(PEInterval(PEId, EndTime, PEIntervals[PEId][Int].EndTime));
  PEIntervals[PEId].erase(PEIntervals[PEId].begin() + Int);
  sort(PEIntervals[PEId].begin(), PEIntervals[PEId].end());
}

Node FindPreviousNode(int NodeId, long long Memory, Node KeyNode, NodeGenerator &ng, bool &InQ) {
  long long PeriodTime = ng.UpBound;
  
  Node ArNode = ng.GetChoosedNode(NodeId, KeyNode.Round);
  long long Com = (PEEdge[KeyNode.PEId][ArNode.PEId] == 0 ? 0 : Ceil(Memory, PEEdge[KeyNode.PEId][ArNode.PEId]));
  long long EndTime = KeyNode.StartTime + KeyNode.Retiming * PeriodTime - Ceil(Memory, CACHESPEED) - Com;
  long long StartTime = EndTime - ArNode.Cost;
  InQ = false;
  if (!ArNode.Certained) {
    InQ = true;
    int TargetInt = -1;
    int PEId = ArNode.PEId;
    for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
      if (PEIntervals[PEId][k].StartTime <= ArNode.StartTime 
        && PEIntervals[PEId][k].EndTime >= ArNode.EndTime) {
        TargetInt = k;
        break;
      }
    }
    if (TargetInt == -1) {
      ArNode.Show();
      ShowInterval(PEId);
    }
    assert(TargetInt != -1);
    PEInterval interval = PEIntervals[PEId][TargetInt];
    // PS + Retiming * P <= S
    ArNode.Retiming = Floor(StartTime - interval.StartTime, PeriodTime);
    EndTime = min(interval.EndTime + ArNode.Retiming * PeriodTime, EndTime);
    StartTime = EndTime - ArNode.Cost;
    long long BinSize = StartTime - (interval.StartTime + ArNode.Retiming * PeriodTime);
    
    vector<Node> SamePENodes = ng.GetSamePEOtherNodes(PEId, NodeId, ArNode.Round, 
                                                      interval.StartTime, 
                                                      interval.EndTime);
    vector<int> NodeSizes;
    for (int k = 0; k < SamePENodes.size(); ++ k)
      NodeSizes.push_back(SamePENodes[k].Cost);
    set<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize, "Greedy");
    long long Sum = 0;
    for (set<int>::iterator it = ArrangedSet.begin(); it != ArrangedSet.end(); ++ it)
      Sum = Sum + NodeSizes[(*it)];

    assert(Sum <= BinSize);
    ArNode.SetTime(interval.StartTime + Sum, interval.StartTime + Sum + ArNode.Cost);

    StartTime = PEIntervals[ArNode.PEId][TargetInt].StartTime;
    EndTime = ArNode.StartTime;
    // printf("Before:[%d, %d]\n", StartTime, EndTime);
    for (set<int>::iterator it = ArrangedSet.begin(); it != ArrangedSet.end(); ++ it) {
      Node NowNode = SamePENodes[(*it)];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNode(NowNode);
    }

    StartTime = ArNode.EndTime;
    EndTime = PEIntervals[ArNode.PEId][TargetInt].EndTime;
    // printf("After:[%d, %d]\n", StartTime, EndTime);
    for (int i = 0; i < SamePENodes.size(); ++ i) {
      if (ArrangedSet.find(i) != ArrangedSet.end())
        continue;
      Node NowNode = SamePENodes[i];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNode(NowNode);
    }
    ArNode.Certained = true;
    DeleteInterval(ArNode.PEId, TargetInt, ArNode.StartTime, ArNode.EndTime);
  }
  else {
    // AS + Retiming * P <= S
    if (ArNode.StartTime + ArNode.Retiming * PeriodTime > StartTime)
      ArNode.Retiming = Floor(StartTime - ArNode.StartTime, PeriodTime);
  }

  return ArNode;
}

void ArrangeKeyNode(Node KeyNode, NodeGenerator &ng, priority_queue<Node, vector<Node>, NodeComparationByCost> &Que) {
  // printf("ArrangeKeyNode\n");
  // KeyNode.Show();
  // deal with in edge
  long long PeriodTime = ng.UpBound;
  vector<Edge> InEdges = ReEdgeList[KeyNode.Id];
  sort(InEdges.begin(), InEdges.end(), CmpEdgeByFromCost);
  for (int i = 0; i < InEdges.size(); ++ i) {
    Edge e = InEdges[i];
    bool InQ = false;
    Node FromNode = FindPreviousNode(e.From, e.Memory, KeyNode, ng, InQ);
    CacheBlock CB = CacheBlock(FromNode.Id, FromNode.Round, KeyNode.Id, 
                            KeyNode.Round, e.Memory, 
                            FromNode.EndTime + FromNode.Retiming * PeriodTime, 
                            KeyNode.StartTime + KeyNode.Retiming * PeriodTime);
    Caches[KeyNode.PEId - 1].AddCacheBlock(CB);
    if (InQ)
      Que.push(FromNode);
    else
      ReChecked[FromNode.Id][FromNode.Round] = true;
    ng.SetNode(FromNode);
    Checked[FromNode.Id][FromNode.Round] = true;
  }
}

void BFS(Node KeyNode, NodeGenerator &ng) {
  // printf("BFS\n");
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  q.push(KeyNode);
  while (!q.empty()) {
    Node SourceNode = q.top();
    q.pop();
    ArrangeKeyNode(ng.GetNode(SourceNode.Id, SourceNode.Round), ng, q);
  }
  // printf("BFS Succeed\n");
}

void PlaceKeyNode(Node &KeyNode, NodeGenerator &ng) {
  if (KeyNode.Certained)
    return ;
  // printf("Place KeyNode\n");
  int Int = -1;
  for (int i = 0; i < PEIntervals[KeyNode.PEId].size(); ++ i) {
    if (PEIntervals[KeyNode.PEId][i].StartTime <= KeyNode.StartTime && KeyNode.EndTime <= PEIntervals[KeyNode.PEId][i].EndTime) {
      Int = i;
      break;
    }
  }
  if (Int == -1) {
    KeyNode.Show();
    ShowInterval(KeyNode.PEId);
    printf("### Not Found KeyNode Interval ###\n");
  }
  assert(Int != -1);
  Checked[KeyNode.Id][KeyNode.Round] = true;
  KeyNode.StartTime = PEIntervals[KeyNode.PEId][Int].StartTime;
  KeyNode.EndTime = KeyNode.StartTime + KeyNode.Cost;
  KeyNode.Retiming = 0;
  KeyNode.Certained = true;
  ng.SetNode(KeyNode);
  vector<Node> OtherNodes = ng.GetSamePEOtherNodes(KeyNode.PEId, KeyNode.Id, KeyNode.Round, 
                                                  PEIntervals[KeyNode.PEId][Int].StartTime, PEIntervals[KeyNode.PEId][Int].EndTime);
  for (int k = 0; k < OtherNodes.size(); ++ k) {
    Node OtherNode = OtherNodes[k];
    OtherNode.SetTime(KeyNode.EndTime, PEIntervals[KeyNode.PEId][Int].EndTime);
    ng.SetNode(OtherNode);
  }

  DeleteInterval(KeyNode.PEId, Int, KeyNode.StartTime, KeyNode.EndTime);  
  // printf("KeyNode Placed\n");
}

void DetectCacheOverflow(NodeGenerator &ng) {
  for (int i = 1; i <= ng.NeedPE; ++ i) {
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    int Index = 0;
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      long long ST = TimeTrace[j];
      long long ED = TimeTrace[j + 1];
      vector<CacheBlock> Blocks;
      long long MemorySum = 0;
      Index = Caches[i - 1].GetCacheBlockByTime(ST, ED, Blocks, MemorySum, Index);
      ng.RunOnCache = ng.RunOnCache + Blocks.size();
      if (MemorySum <= CACHESIZE)
        continue;

      vector<int> Memory;
      for (int k = 0; k < Blocks.size(); ++ k)
        Memory.push_back(Blocks[k].Memory);
      set<int> ArrangedSet = ArrangeInFixedSize(Memory, CACHESIZE, "Greedy");
      
      for (int k = 0; k < Blocks.size(); ++ k) {
        if (ArrangedSet.find(k) != ArrangedSet.end())
          continue;
        CacheBlock CB = Blocks[k];
        Caches[i - 1].DeleteCacheBlock(CB);
        ng.RunOnCache = ng.RunOnCache - 1;
        DRAMBlocks.push_back(CB);
        ReChecked[CB.NodeIds.second][CB.Rounds.second] = true;
      }
    }
  }
  ng.RunOnDRAM = DRAMBlocks.size();
  long long CacheMemorySum = 0;
  for (int i = 1; i <= ng.NeedPE; ++ i) {
    Caches[i - 1].SortCacheBlock();
    vector<long long> TimeTrace = Caches[i - 1].GetTimeTrace();
    int Index = 0;
    for (int j = 0; j < TimeTrace.size() - 1; ++ j) {
      long long ST = TimeTrace[j];
      long long ED = TimeTrace[j + 1];
      vector<CacheBlock> Blocks;
      long long MemorySum = 0;
      Index = Caches[i - 1].GetCacheBlockByTime(ST, ED, Blocks, MemorySum, Index);
      CacheMemorySum = CacheMemorySum + MemorySum * (ED - ST);
      assert(MemorySum <= CACHESIZE);
    }
  }
  ng.CacheRatio = (CacheMemorySum * 1.0) / (CACHESIZE * ng.UpBound * ng.NeedPE);
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

void ReBFS(Node KeyNode, NodeGenerator &ng) {
  // printf("ReBFS\n");
  long long PeriodTime = ng.UpBound;
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
      Node FromNode = ng.GetNode(InEdges[i].From, ToNode.Round);
      long long Com = (PEEdge[FromNode.PEId][ToNode.PEId] == 0 ? 0 : Ceil(e.Memory, PEEdge[FromNode.PEId][ToNode.PEId]));
      long long Cost = (GetStrogePos(FromNode.Id, FromNode.Round, ToNode.Id, ToNode.Round) 
                      ? Ceil(e.Memory, CACHESPEED) : Ceil(e.Memory, DRAMSPEED)) + Com;
      if (FromNode.EndTime + FromNode.Retiming * PeriodTime + Cost > ToNode.StartTime + ToNode.Retiming * PeriodTime) {
        // FE + Retiming * P <= TS
        FromNode.Retiming = Floor(ToNode.StartTime + ToNode.Retiming * PeriodTime - FromNode.EndTime, PeriodTime);
        ng.SetNode(FromNode);
        q.push(FromNode);
      }
    }
  }
}

void SpreadKeyNodeSet(NodeGenerator &ng) {
  for (int i = 1; i <= ng.NeedPE; ++ i) {
    PEIntervals[i].clear();
    PEIntervals[i].push_back(PEInterval(i, 0, ng.UpBound));
  }
  vector<Node> UnCheckedNodes;
  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= ng.UpRound; ++ j)
      UnCheckedNodes.push_back(ng.GetNode(i, j));

  do {
    vector<Node> KeyNodeSet = GetKeyNodeSet(UnCheckedNodes);

    for (int i = 0; i < KeyNodeSet.size(); ++ i) {
      Node KeyNode = KeyNodeSet[i];
      KeyNode = ng.GetNode(KeyNode.Id, KeyNode.Round);
      if (!Checked[KeyNode.Id][KeyNode.Round]) {
        PlaceKeyNode(KeyNode, ng);
        BFS(KeyNode, ng);
      }
    }
  } while (!UnCheckedNodes.empty());

  DetectCacheOverflow(ng);

  vector<Node> ReCheckedNodes;
  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= ng.UpRound; ++ j)
      if (ReChecked[i][j]) ReCheckedNodes.push_back(ng.GetNode(i, j));
  
  sort(ReCheckedNodes.begin(), ReCheckedNodes.end(), CmpByTopoOrder);
  // printf("ReChecked Nodes\n");
  for (int i = ReCheckedNodes.size() - 1; i >= 0; -- i) {
    Node KeyNode = ReCheckedNodes[i];
    if (ReChecked[KeyNode.Id][KeyNode.Round])
      ReBFS(ng.GetNode(KeyNode.Id, KeyNode.Round), ng);
  }
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);
  for (int i = 0; i < NgList.size(); ++ i) {
    // printf("\nUpBound:%d\tUpRound:%d\n", NgList[i].UpBound, NgList[i].UpRound);
    memset(Checked, false, sizeof(Checked));
    Caches.clear();
    DRAMBlocks.clear();
    for (int j = 1; j <= NgList[i].NeedPE; ++ j) {
      CacheManager CM = CacheManager();
      CM.PEId = j;
      Caches.push_back(CM);
    }

    SpreadKeyNodeSet(NgList[i]);
    NgList[i].CalcPrelogue();
    NgList[i].AllRound = PeriodTimes;
    // printf("PE:%2d\tDiv:%2d\tUpRound:%2d\tUpBound:%5d\tRetiming:%2d\tBoundPerRound:%d\tPrelogue:%lld\n", NgList[i].NeedPE, NgList[i].Div, NgList[i].UpRound, NgList[i].UpBound, NgList[i].Retiming, NgList[i].UpBound / NgList[i].UpRound, NgList[i].Prelogue);
    // printf("%d\t%lld\n", NgList[i].AllRound, NgList[i].Prelogue + (Ceil(Ceil(NgList[i].AllRound, NgList[i].Div), NgList[i].UpRound) - 1) * NgList[i].UpBound);
  }

  sort(NgList.begin(), NgList.end(), [](NodeGenerator a, NodeGenerator b) {
    return a.Prelogue + (Ceil(Ceil(a.AllRound, a.Div), a.UpRound) - 1) * a.UpBound < b.Prelogue + (Ceil(Ceil(b.AllRound, b.Div), b.UpRound) - 1) * b.UpBound;
  });

  priority_queue<NodeGenerator, vector<NodeGenerator>, NodeGeneratorComparator> q;
  int RestPE = TotalPE;
  for (int i = 0; i < NgList.size() && RestPE > 0; ++ i) {
    if (NgList[i].NeedPE > RestPE)
      continue;
    q.push(NgList[i]);
    RestPE = RestPE - (RestPE / NgList[i].NeedPE) * NgList[i].NeedPE;
    // printf("PUSH:%d\t%d\n", NgList[i].NeedPE, RestPE);
  }

  int TotalRound = PeriodTimes;
  RestPE = TotalPE;
  while (TotalRound > 0 && !q.empty()) {
    NodeGenerator ng = q.top();
    q.pop();
    if (ng.TotalRound == 0) {
      if(ng.NeedPE > RestPE)
        continue;
      else {
        ng.Div = RestPE / ng.NeedPE;
        RestPE = RestPE - ng.NeedPE * ng.Div;
      }
    }
    TotalRound = TotalRound - ng.UpRound * ng.Div;
    ng.PlaceOnePeriod();
    q.push(ng);
  }

  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;

  vector<NodeGenerator> res;
  while (!q.empty()) {
    // printf("Schedule:%d\tRound:%d\tTotalTime:%lld\n", q.top().NeedPE, q.top().TotalRound, q.top().TotalTime);
    res.push_back(q.top());
    q.pop();
  }

  RestPE = 0;
  for (int i = 0; i < res.size(); ++ i)
    RestPE = RestPE + res[i].NeedPE * res[i].Div;
  assert(RestPE <= TotalPE);


  FinalResult FR = FinalResult();
  long long TotalTime = 0;
  double MaxRatio = 0.;
  int used = 0;
  for (int i = 0; i < res.size(); ++ i) {
    // printf("%d\t%8d\t%.6f\n", res[i].NeedPE, res[i].Retiming * res[i].UpBound, res[i].UpBound * 1. / res[i].UpRound);
    // printf("%d\t%d\t%lld\n", res[i].NeedPE, res[i].TotalRound, res[i].TotalTime);
    if (res[i].TotalRound == 0)
      continue;
    used = used + 1;
    FR.TotalTime = max(FR.TotalTime, res[i].TotalTime);
    if (res[i].Prelogue > FR.Prelogue) {
      FR.Prelogue = res[i].Prelogue;
      FR.Retiming = res[i].Retiming;
    }
    FR.RunOnCache = FR.RunOnCache + res[i].RunOnCache * res[i].TotalRound * res[i].Div;
    FR.RunOnDRAM = FR.RunOnDRAM + res[i].RunOnDRAM * res[i].TotalRound * res[i].Div;
    MaxRatio = MaxRatio + res[i].MaxRatio;
  }
  FR.CPURatio = (1.0 * PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  FR.MAXRatio = MaxRatio / used;
  return FR;
}