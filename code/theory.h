struct Node {
  int Id;
  int Cost;

  int PEId;       // run on which pe
  int Round;
  int StartTime;
  int EndTime;

  Node() {
    Round = PEId = -1;
    StartTime = EndTime = -1;
  }

  Node(int a, int b) {
    Id = a;
    Cost = b;
    Round = PEId = -1;
    StartTime = EndTime = -1;
  }

  void SetTime(int st, int ed) {
    StartTime = st;
    EndTime = ed;
  }

  void Copy(Node a) {
    Id = a.Id;
    Cost = a.Cost;

    PEId = a.PEId;
    Round = a.Round;
    StartTime = a.StartTime;
    EndTime = a.EndTime;
  }

  void Show() {
    printf("ID:%d\tPE:%d\tRound:%d\tST:%d\tED:%d\tCost:%d\n", Id, PEId, Round, StartTime, EndTime, Cost);
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
    DRAMTimeCost = ceil(1.0 * Memory / DRAMTimeCost);
  }
};

int MapTopology[MAXN];

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
  else if (MapTopology[a.Id] != MapTopology[b.Id])
    return MapTopology[a.Id] < MapTopology[b.Id];
  else if (a.Round != b.Round)
    return a.Round < b.Round;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
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
    if (a.Cost != b.Cost)
      return a.Cost < b.Cost;
    else if (a.Round != b.Round)
      return a.Round > b.Round;
    return a.Id > b.Id;    
  }
};

struct PEInterval {
  int PEId;
  int StartTime;
  int EndTime;

  PEInterval() { }

  PEInterval(int a, int b, int c) {
    PEId = a;
    StartTime = b;
    EndTime = c;
  }

  void SetTime(int a, int b) {
    StartTime = a;
    EndTime = b;
  }

  bool Include(int l, int r) {
    if (StartTime <= l && r <= EndTime)
      return true;
    return false;
  }

  friend bool operator < (PEInterval a, PEInterval b) {
    if (a.PEId != b.PEId)
      return a.PEId < b.PEId;
    return a.StartTime < b.StartTime;
  }
};

struct NodeGenerator {
  int TotalNode;
  int NeedPE;
  int UpBound;
  int UpRound;
  int Prelogue;
  vector<Node> StartTable;
  int PELoc[MAXPE];

  NodeGenerator() {
    TotalNode = 0;
    NeedPE = 0;
    UpBound = 0;
    Prelogue = -1;
    StartTable.clear();
  }

  NodeGenerator(int a, int b, int MaxRound, Node NodeList[MAXN]) {
    TotalNode = a;
    NeedPE = b;
    UpBound = 0;
    Prelogue = -1;
    StartTable.clear();
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
        UpBound = max(UpBound, Emp.EndTime + n.Cost);
        n.SetTime(-1, -1);
        StartTable.push_back(n);
      }
    }
    sort(StartTable.begin(), StartTable.end(), CmpByPE);
    memset(PELoc, 0, sizeof(PELoc));
    int LastPEId = -1;
    for (int i = 0; i < StartTable.size(); ++ i) {
      if (StartTable[i].PEId != LastPEId) {
        PELoc[StartTable[i].PEId] = i;
        LastPEId = StartTable[i].PEId;
      }
    }
    PELoc[StartTable[StartTable.size() - 1].PEId + 1] = StartTable.size();
    // calculate the use ratio of cpu
    assert(UpBound != 0);
    double Down = UpBound * NeedPE;
    double Up = 0;
    for (int i = 1; i <= TotalNode; ++ i) {
      Up = Up + NodeList[i].Cost;
    }
    Up = Up * UpRound;
    sort(NodeList + 1, NodeList + TotalNode + 1, CmpById);
    assert(Down != 0);
    double Ratio = Up / Down;
    return Ratio;
  }

  void CalcBound(int MaxRound, Node NodeList[MAXN]) {
    int TargetRound = 1;
    double MaxRatio = 0;
    for (UpRound = 1; UpRound <= MaxRound; ++ UpRound) {
      double NowRatio = Init(NodeList);
      if (NowRatio >= LIMITEDRATIO) {
        return ;
      }
      else if (NowRatio > MaxRatio) {
        TargetRound = UpRound;
        MaxRatio = NowRatio;
      }
    }
    UpRound = TargetRound;
    Init(NodeList);
  }

  TwoInt BinarySearch(int NodeId) {
    sort(StartTable.begin(), StartTable.end(), CmpById);
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

  vector<int> GetNodeInRounds(int NodeId, bool Checked[MAXN][MAXR]) {
    vector<int> Rounds;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i)
      if (StartTable[i].Id == NodeId && !Checked[NodeId][StartTable[i].Round])
        Rounds.push_back(StartTable[i].Round);
    sort(StartTable.begin(), StartTable.end(), CmpByPE);
    return Rounds;
  }

  vector<Node> GetSamePEOtherNodes(int PEId, int NodeId, int Round) {
    vector<Node> Nodes;
    for (int i = PELoc[PEId]; i < PELoc[PEId + 1]; ++ i) {
      if (StartTable[i].StartTime != -1)
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

  void SetNodeTime(int NodeId, int Round, int StartTime, int EndTime) {
    bool Found = false;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      if (StartTable[i].Round == Round) {
        StartTable[i].SetTime(StartTime, EndTime);
        Found = true;
        break;
      }
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
    printf("Not Found Node:%d %d\n", NodeId, Round);
    assert(1 == 0);
    return Node();
  }

  // Node findInStartTable(int numb, int id) {
  //   int l = 0, r = StartTable.size();
  //   int stindex = 0;
  //   assert(StartTable.size() != 0);
  //   if (StartTable[0].id != id) {
  //     while (r - l > 1) {
  //       int m = (l + r) >> 1;
  //       if (StartTable[m].id < id)
  //         l = m;
  //       else
  //         r = m;
  //     }
  //     stindex = r;
  //   }
  //   Node res = Node();
  //   while (stindex < StartTable.size()) {
  //     if (StartTable[stindex].round == numb) {
  //       res.Copy(StartTable[stindex]);
  //       break;
  //     }
  //     stindex = stindex + 1;
  //   }
  //   assert(res.PEId != -1);
  //   return res;
  // }

  // Node generateNextNode(Node n, Node NodeList[MAXN]) {
  //   Node res = Node(n.id, n.Cost);
  //   res.Copy(n);
  //   res.numbinq = res.numbinq + 1;
  //   assert(UpRound != 0);
  //   int base = (res.numbinq % UpRound == 0 ? res.numbinq / UpRound - 1 : res.numbinq / UpRound);
  //   int numb = (res.numbinq - 1) % UpRound + 1;
  //   res.round = base + 1;
  //   // PP p = getStartTime(res.id, numb, NodeList);
  //   // res.StartTime = base * UpBound + p.first;
  //   Node f = findInStartTable(numb, res.id);
  //   res.StartTime = base * UpBound + f.StartTime;
  //   res.endtime = res.StartTime + res.Cost;
  //   res.PEId = f.PEId;
  //   return res;
  // }

  // void test(Node NodeList[MAXN]) {
  //   int test_round = 9;
  //   for (int j = 1; j <= TotalNode; ++ j) {
  //     Node st = Node();
  //     st.Copy(NodeList[j]);
  //     st.setTime(0, 0);
  //     st.numbinq = 0;
  //     printf("TEST Node:%d Cost:%.3f\n", NodeList[j].id, NodeList[j].Cost);
  //     for (int i = 0; i < test_round; ++ i) {
  //       st.Copy(generateNextNode(st, NodeList));
  //       printf("Numb:%d\tST:%.3f\tED:%.3f\tPE:%d\n", st.numbinq, st.StartTime, st.endtime, st.PEId);
  //     }
  //     printf("\n");
  //   }
  // }

  void Show() {
    sort(StartTable.begin(), StartTable.end(), CmpByPE);
    int LastPEId = -1;
    printf("\nArrangement");
    for (int i = 0; i < StartTable.size(); ++ i) {
      if (StartTable[i].PEId != LastPEId) {
        LastPEId = StartTable[i].PEId;
        printf("\n");
      }
      for (int j = 0; j < StartTable[i].Cost; ++ j)
        printf("%c", ('A' + StartTable[i].Id - 1));
    }
    printf("\n");
  }
};

vector<NodeGenerator> NgList;
int Topology[MAXN];
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;
vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
Node NodeList[MAXN];
int NeedPE;
int DP[MAXN];
TwoInt Trace[MAXN];
vector<PEInterval> PEIntervals[MAXPE];
bool Checked[MAXN][MAXR];
int Cache[MAXPE][MAXM << 2];
int Lazy[MAXPE][MAXM << 2];


void PushUp(int p, int rt) {
  Cache[p][rt] = max(Cache[p][rt << 1], Cache[p][rt << 1 | 1]);
}

void PushDown(int p, int rt) {
  if (Lazy[p][rt] > 0) {
    Cache[p][rt << 1] = Cache[p][rt << 1] + Lazy[p][rt];
    Cache[p][rt << 1 | 1] = Cache[p][rt << 1 | 1] + Lazy[p][rt];
    Lazy[p][rt << 1] = Lazy[p][rt << 1] + Lazy[p][rt];
    Lazy[p][rt << 1 | 1] = Lazy[p][rt << 1 | 1] + Lazy[p][rt];
    Lazy[p][rt] = 0;
  }
}

void Build(int p, int l, int r, int rt) {
  if (l == r) {
    Cache[p][rt] = 0;
    return ;
  }
  Lazy[p][rt] = 0;
  int m = (l + r) << 1;
  Build(p, l, m, rt << 1);
  Build(p, m + 1, r, rt << 1 | 1);
  PushUp(p, rt);
}

void Update(int p, int l, int r, int rt, int L, int R, int add) {
  if (L <= l && r <= R) {
    Cache[p][rt] = Cache[p][rt] + add;
    Lazy[p][rt] = add;
    return ;
  }
  PushDown(p, rt);
  int m = (l + r) >> 1;
  if (L <= m)
    Update(p, l, m, rt << 1, L, R, add);
  if (R > m)
    Update(p, m + 1, r, rt << 1 | 1, L, R, add);
  PushUp(p, rt);
}

int Query(int p, int l, int r, int rt, int L, int R) {
  if (L <= l && r <= R) {
    return Cache[p][rt];
  }
  PushDown(p, rt);
  int m = (l + r) >> 1;
  int Max = 0;
  if (L <= m)
    Max = max(Max, Query(p, l, m, rt << 1, L, R));
  if (R > m)
    Max = max(Max, Query(p, m + 1, r, rt << 1 | 1, L, R));
  PushUp(p, rt);
  return Max;
}

void GetTopology() {
  int Count = 0, Iter = 0, Order = 0;
  NeedPE = 0;
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
    Topology[Iter] = f.Id;
    MapTopology[f.Id] = Order;

    Iter = Iter + 1;
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
}

void Init(int TotalPE, int UpRound) {
  memset(Topology, 0, sizeof(Topology));
  memset(Degree, 0, sizeof(Degree));
  memset(Checked, false, sizeof(Checked));

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  GetTopology();

  NgList.push_back(NodeGenerator(TotalNode, NeedPE, UpRound, NodeList));
  if (TotalPE % NeedPE != 0) {
    NgList.push_back(NodeGenerator(TotalNode, TotalPE % NeedPE, UpRound, NodeList));
  }
}

vector<Node> GetKeyNodeSet() {
  vector<Node> KeyNodeSet;
  int MaxCost = -1;
  for (int i = 1; i <= TotalNode; ++ i)
    MaxCost = max(MaxCost, NodeList[i].Cost);

  for (int i = 1; i <= TotalNode; ++ i) {
    if (NodeList[i].Cost >= MaxCost * ALPHA)
      KeyNodeSet.push_back(NodeList[i]);
  }

  sort(KeyNodeSet.begin(), KeyNodeSet.end(), CmpByCost);
  return KeyNodeSet;
}

vector<int> ArrangeInFixedSize(vector<int> Goods, int BinSize) {
  for (int i = 0; i <= BinSize; ++ i) {
    DP[i] = 0;
    Trace[i] = make_pair(i, i);
  }

  for (int i = 0; i < Goods.size(); ++ i) {
    for (int j = BinSize; j >= Goods[i]; -- j) {
      if (DP[j - Goods[i]] + Goods[i] > DP[j]) {
        DP[j] = DP[j - Goods[i]] + Goods[i];
        Trace[j] = make_pair(j - Goods[i], i);
      }
    }
  }
  vector<int> ArrangedGoods;
  int k = BinSize;
  while (Trace[k].first != k) {
    ArrangedGoods.push_back(Trace[k].second);
    k = Trace[k].first;
  }
  return ArrangedGoods;  
}

bool ArrangeConnectedNode(int FromNodeId, int KeyNodeTime, int KeyNodePEId, int Direction, int CacheCost, int DRAMCost, NodeGenerator &ng, Node &FromNode) {
  vector<int> Rounds = ng.GetNodeInRounds(FromNodeId, Checked);
  if (Rounds.size() == 0)
    return false;

  bool PlaceInCache = true;

  int TargetStartTime = -1;
  int TargetPEId = -1;
  int TargetRound = -1;
  int TargetInt = -1;
  int PeriodTime = ng.UpBound;
  // 选取最近的PE
  for (int j = 0; j < Rounds.size(); ++ j) {
    Node FromNode = ng.GetNode(FromNodeId, Rounds[j]);
    int PEId = FromNode.PEId;

    int StartTime = KeyNodeTime - Direction * (CacheCost + NodeList[FromNodeId].Cost);
    int EndTime = StartTime + Direction * NodeList[FromNodeId].Cost;
    if (StartTime > EndTime)
      swap(StartTime, EndTime);
    while (StartTime < 0) {
      StartTime = StartTime + PeriodTime;
      EndTime = EndTime + PeriodTime;
    }
    
    int Int = -1;
    for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
      if (PEIntervals[PEId][k].Include(StartTime, EndTime)) {
        Int = k;
        break;
      }
    }
    
    if (Int == -1) {
      printf("%d %d %d\n", FromNodeId, Rounds[j], NodeList[FromNodeId].Cost);
      printf("%d %d\n", PEId, (int)PEIntervals[PEId].size());
      for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
        if (PEIntervals[PEId][k].EndTime - PEIntervals[PEId][k].StartTime >= NodeList[FromNodeId].Cost) {
          if (Direction == 1 && PEIntervals[PEId][k].EndTime <= StartTime) {
            Int = k;
          }
          else if (Direction == -1 && PEIntervals[PEId][k].StartTime >= StartTime + NodeList[FromNodeId].Cost) {
            Int = k;
            break;
          }
        }
      }
      assert(Int != -1);
      if (Direction == 1) {
        EndTime = PEIntervals[PEId][Int].EndTime;
        StartTime = EndTime - NodeList[FromNodeId].Cost;
      }
      else {
        StartTime = PEIntervals[PEId][Int].StartTime;
        EndTime = StartTime + NodeList[FromNodeId].Cost;
      }
    }
    else {
      int BinSize = (Direction == 1 ? StartTime - PEIntervals[PEId][Int].StartTime : PEIntervals[PEId][Int].EndTime - StartTime - NodeList[FromNodeId].Cost);
      vector<Node> SamePENodes = ng.GetSamePEOtherNodes(PEId, FromNodeId, Rounds[j]);
    
      vector<int> NodeSizes;
      for (int k = 0; k < SamePENodes.size(); ++ k)
        NodeSizes.push_back(SamePENodes[k].Cost);
    
      vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
    
      int Sum = 0;
      for (int k = 0; k < ArrangedSet.size(); ++ k)
        Sum = Sum + NodeSizes[ArrangedSet[k]];

      assert(Sum <= BinSize);
      if (Direction == 1) {
        StartTime = PEIntervals[PEId][Int].StartTime + Sum;
        EndTime = StartTime + NodeList[FromNodeId].Cost;
      }
      else {
        EndTime = PEIntervals[PEId][Int].EndTime - Sum;
        StartTime = EndTime - NodeList[FromNodeId].Cost;
      }
    }

    if (Direction == 1 && (TargetStartTime == -1 || StartTime > TargetStartTime)) {
      TargetStartTime = StartTime;
      TargetPEId = PEId;
      TargetRound = Rounds[j];
      TargetInt = Int; 
    }
    else if (Direction == -1 && (TargetStartTime == -1 || StartTime < TargetStartTime)) {
      TargetStartTime = StartTime;
      TargetPEId = PEId;
      TargetRound = Rounds[j];
      TargetInt = Int;       
    }
  }

  assert(TargetRound != -1);
  if (Direction == 1)
    PlaceInCache = (KeyNodeTime - TargetStartTime - NodeList[FromNodeId].Cost) < DRAMCost;
  else
    PlaceInCache = (TargetStartTime - KeyNodeTime) < DRAMCost;
  FromNode = ng.GetNode(FromNodeId, TargetRound);
  if (KeyNodePEId != -1 && TargetPEId != KeyNodePEId)
    return false;
  if (PEIntervals[TargetPEId][TargetInt].StartTime < TargetStartTime)
    PEIntervals[TargetPEId].push_back(PEInterval(TargetPEId, PEIntervals[TargetPEId][TargetInt].StartTime, TargetStartTime));
  if (TargetStartTime + NodeList[FromNodeId].Cost < PEIntervals[TargetPEId][TargetInt].EndTime)
    PEIntervals[TargetPEId].push_back(PEInterval(TargetPEId, TargetStartTime + NodeList[FromNodeId].Cost, PEIntervals[TargetPEId][TargetInt].EndTime));
  PEIntervals[TargetPEId].erase(PEIntervals[TargetPEId].begin() + TargetInt);
  sort(PEIntervals[TargetPEId].begin(), PEIntervals[TargetPEId].end());

  Checked[FromNodeId][TargetRound] = true;
  ng.SetNodeTime(FromNodeId, TargetRound, TargetStartTime, TargetStartTime + NodeList[FromNodeId].Cost);
  FromNode = ng.GetNode(FromNodeId, TargetRound);
  return PlaceInCache;
}

void ArrangeKeyNode(Node KeyNode, NodeGenerator ng) {
  printf("Arrange KeyNode:%d Round:%d\n", KeyNode.Id, KeyNode.Round);
  assert(KeyNode.PEId > 0 && KeyNode.PEId <= ng.NeedPE);
  // arrange keynode position
  bool Placed = false;
  int KeyNodeStartTime = -1;
  for (int i = 0; i < PEIntervals[KeyNode.PEId].size(); ) {
    if (PEIntervals[KeyNode.PEId][i].EndTime - PEIntervals[KeyNode.PEId][i].StartTime >= KeyNode.Cost) {
      KeyNodeStartTime = PEIntervals[KeyNode.PEId][i].StartTime;
      Checked[KeyNode.Id][KeyNode.Round] = true;
      ng.SetNodeTime(KeyNode.Id, KeyNode.Round, PEIntervals[KeyNode.PEId][i].StartTime, PEIntervals[KeyNode.PEId][i].StartTime + KeyNode.Cost);
      if (PEIntervals[KeyNode.PEId][i].EndTime - PEIntervals[KeyNode.PEId][i].StartTime == KeyNode.Cost) {
        PEIntervals[KeyNode.PEId].erase(PEIntervals[KeyNode.PEId].begin() + i);
      }
      else {
        PEIntervals[KeyNode.PEId][i].SetTime(PEIntervals[KeyNode.PEId][i].StartTime + KeyNode.Cost, PEIntervals[KeyNode.PEId][i].EndTime);
        ++ i;
      }
      Placed = true;
      break;
    }
    else {
      ++ i;
    }
  }
  int KeyNodeEndTime = KeyNodeStartTime + KeyNode.Cost;
  assert(Placed == true); 

  vector<Edge> Edges;
  vector<ThreeInt> ReadyForCache;
  int PeriodTime = ng.UpBound;
  int MaxCost = 0;
  // deal with in edge
  Edges = ReEdgeList[KeyNode.Id];
  for (int i = 0; i < Edges.size(); ++ i) {
    int FromNodeId = Edges[i].From;
    printf("Arrange In-Edge:%d %d\n", FromNodeId, KeyNode.Id);
    Node FromNode = Node();
    if (ArrangeConnectedNode(FromNodeId, KeyNodeStartTime, -1, 1, Edges[i].CacheTimeCost, Edges[i].DRAMTimeCost, ng, FromNode))
      ReadyForCache.push_back(make_pair(FromNodeId, make_pair(FromNode.Round, Edges[i].Memory)));
    MaxCost = max(MaxCost, KeyNodeStartTime - FromNode.StartTime - NodeList[FromNodeId].Cost);
  }
  int R = KeyNodeStartTime;
  int L = KeyNodeStartTime - MaxCost;
  int H = Query(KeyNode.PEId, 0, PeriodTime, 1, L, R);
  int BinSize = CACHESIZE - H;
  
  vector<int> NodeSizes;
  for (int i = 0; i < ReadyForCache.size(); ++ i) {
    NodeSizes.push_back(NodeList[ReadyForCache[i].first].Cost);
  }
  vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);

  for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
    int NodeId = ReadyForCache[ArrangedSet[i]].first;
    int Round = ReadyForCache[ArrangedSet[i]].second.first;
    int Cost = KeyNodeStartTime - ng.GetNode(NodeId, Round).StartTime - NodeList[NodeId].Cost;
    Update(KeyNode.PEId, 0, PeriodTime, 1, R - Cost, R, ReadyForCache[ArrangedSet[i]].second.second);
    Checked[NodeId][Round] = true;
    ng.SetNodeTime(NodeId, Round, -1, -1);
  }

  // deal with out edge whose pe's id is same
  Edges.clear();
  ReadyForCache.clear();
  MaxCost = 0;
  // deal with in edge
  Edges = EdgeList[KeyNode.Id];
  for (int i = 0; i < Edges.size(); ++ i) {
    int ToNodeId = Edges[i].To;
    printf("Arrange Out-Edge:%d %d\n", KeyNode.Id, ToNodeId);
    Node ToNode = Node();
    if (ArrangeConnectedNode(ToNodeId, KeyNodeEndTime, KeyNode.PEId, -1, Edges[i].CacheTimeCost, Edges[i].DRAMTimeCost, ng, ToNode))
      ReadyForCache.push_back(make_pair(ToNodeId, make_pair(ToNode.Round, Edges[i].Memory)));
    MaxCost = max(MaxCost, ToNode.StartTime - KeyNodeEndTime);
  }
  L = KeyNodeEndTime;
  R = KeyNodeEndTime + MaxCost;
  H = Query(KeyNode.PEId, 0, PeriodTime, 1, L, R);
  BinSize = CACHESIZE - H;
  
  NodeSizes.clear();
  for (int i = 0; i < ReadyForCache.size(); ++ i) {
    NodeSizes.push_back(NodeList[ReadyForCache[i].first].Cost);
  }
  ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);

  for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
    int NodeId = ReadyForCache[ArrangedSet[i]].first;
    int Round = ReadyForCache[ArrangedSet[i]].second.first;
    int Cost = ng.GetNode(NodeId, Round).StartTime - KeyNodeEndTime;
    Update(KeyNode.PEId, 0, PeriodTime, 1, L, L + Cost, ReadyForCache[ArrangedSet[i]].second.second);
    Checked[NodeId][Round] = true;
    ng.SetNodeTime(NodeId, Round, -1, -1);
  }
}

void SpreadKeyNodeSet(vector<Node> KeyNodeSet, NodeGenerator &ng) {
  for (int i = 1; i <= ng.NeedPE; ++ i) 
    PEIntervals[i].push_back(PEInterval(i, 0, ng.UpBound));
  for (int i = 0; i < KeyNodeSet.size(); ++ i) {
    vector<int> Rounds = ng.GetNodeInRounds(KeyNodeSet[i].Id, Checked);
    for (int j = 0; j < Rounds.size(); ++ j) {
      Node KeyNode = ng.GetNode(KeyNodeSet[i].Id, Rounds[j]);
      ArrangeKeyNode(KeyNode, ng);
    }
  }
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  for (int i = 1; i <= TotalNode; ++ i) {
    bool IsKeyNode = false;
    for (int j = 0; j < KeyNodeSet.size(); ++ j) {
      if (KeyNodeSet[j].Id == NodeList[i].Id) {
        IsKeyNode = true;
        break;
      }
    }
    if (!IsKeyNode) {
      for (int j = 1; j <= ng.UpRound; ++ j) {
        Node node = ng.GetNode(i, j);
        q.push(node);
      }
    }
  }
  while (!q.empty()) {
    Node KeyNode = q.top();
    q.pop();
    if (Checked[KeyNode.Id][KeyNode.Round])
      continue;
    ArrangeKeyNode(KeyNode, ng);
  }
}

void Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);
  vector<Node> KeyNodeSet = GetKeyNodeSet();
  for (int i = 0; i < NgList.size(); ++ i) {
    printf("UpBound:%d\n", NgList[i].UpBound);
    NgList[i].Show();
    SpreadKeyNodeSet(KeyNodeSet, NgList[i]);

  }
}