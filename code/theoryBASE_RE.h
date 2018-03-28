
Node NodeList[MAXN];

struct CertainedEdge {
  int FromId;
  int FromRound;
  int ToId;
  int ToRound;
  bool StrogeInCache;

  CertainedEdge(int a, int b, int c, int d, bool e) {
    FromId = a;
    FromRound = b;
    ToId = c;
    ToRound = d;
    StrogeInCache = e;
  }

  void Show() {
    printf("From-Id:%d\tRound:%d\tTo-Id:%d\tRound:%d\tInCache:%d\n", FromId, FromRound, ToId, ToRound, StrogeInCache);
  }

  friend bool operator< (CertainedEdge a, CertainedEdge b) {
    if (a.FromId != b.FromId)
      return a.FromId < b.FromId;
    else if (a.ToId != b.ToId)
      return a.ToId < b.ToId;
    else if (a.ToRound != b.ToRound)
      return a.ToRound < b.ToRound;
    return a.FromRound < b.FromRound;
  }
};

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

struct NodeGenerator {
  int TotalNode;
  int NeedPE;
  int UpBound;
  int UpRound;
  long long Prelogue;
  int Retiming;
  int RunOnCache;
  int RunOnDRAM;
  vector<Node> StartTable;
  vector<CertainedEdge> Relation;

  NodeGenerator() {
    TotalNode = 0;
    NeedPE = 0;
    UpBound = 0;
    Prelogue = -1;
    RunOnCache = RunOnDRAM = 0;
    StartTable.clear();
    Relation.clear();
  }

  NodeGenerator(int a, int b, int MaxRound, Node NodeList[MAXN]) {
    TotalNode = a;
    NeedPE = b;
    UpBound = 0;
    Prelogue = -1;
    RunOnCache = RunOnDRAM = 0;
    StartTable.clear();
    Relation.clear();
    CalcBound(MaxRound, NodeList);
  }

  double Init(Node NodeList[MAXN]) {
    queue<Node> q;
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
        Node Emp = q.front();
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
    double MaxRatio = 0;
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
    Init(NodeList);
    for (int i = 0; i < StartTable.size(); ++ i)
      StartTable[i].SetTime(0, UpBound);
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

  void AddRelation(CertainedEdge e) {
    Relation.push_back(e);
  }

  void SortRealtion() {
    sort(Relation.begin(), Relation.end());
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

  int GetRelatedNode(int FromId, int ToId, int ToRound, bool &StrogeInCache) {
    // printf("GetRelatedNode\tFrom Id:%d\tTo Id:%d\tTo Round:%d\n", FromId, ToId, ToRound);
    if (Relation.size() == 0)
      return -1;
    int L = 0;
    int R = Relation.size() - 1;
    if (Relation[L].FromId > FromId)
      return -1;
    if (Relation[R].FromId < FromId)
      return -1;
    if (Relation[L].FromId != FromId) {
      while (R - L > 1) {
        int M = (L + R) / 2;
        if (Relation[M].FromId < FromId) L = M;
        else R = M;
      }
      if (Relation[R].FromId > FromId)
        return -1;
      L = R;
    }
    for (; L < Relation.size() && Relation[L].FromId == FromId; ++ L) {
      assert(Relation[L].FromId == FromId);
      if (Relation[L].ToId == ToId && Relation[L].ToRound == ToRound)
        break;
    }
    assert(Relation[L].ToId == ToId);
    assert(Relation[L].ToRound == ToRound);
    StrogeInCache = Relation[L].StrogeInCache;
    return Relation[L].FromRound;
  }

  bool FindRelation(int FromId, int FromRound, int ToId) {
    for (int i = 0; i < Relation.size(); ++ i) {
      CertainedEdge e = Relation[i];
      if (e.FromId == FromId && e.FromRound == FromRound && e.ToId == ToId)
        return true;
    }
    return false;
  }

  vector<Node> GetChoosedNode(int NodeId, int KeyNodeId) {
    vector<Node> ChoosedNodes;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      assert (StartTable[i].Id == NodeId);
      int Round = StartTable[i].Round;
      if (!FindRelation(NodeId, Round, KeyNodeId))
        ChoosedNodes.push_back(StartTable[i]);
    }
    return ChoosedNodes;
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
    Retiming = MaxRetiming - MinRetiming + 1;
    Prelogue = Retiming * UpBound;
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

  void ShowRelation() {
    for (int i = 0; i < Relation.size(); ++ i) {
      Relation[i].Show();
    }
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

vector<NodeGenerator> NgList;
vector<Iteration> IterList;
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;

vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
vector<PEInterval> PEIntervals[MAXPE];

int DP[MAXN][MAXSIZE];
int Cache[MAXPE][MAXM << 2];
int Lazy[MAXPE][MAXM << 2];

bool Checked[MAXN][MAXR];
bool ReChecked[MAXN][MAXR];

Node NodeTime[MAXPE][MAXN];

void ShowInterval(int PEId) {
  printf("PEId:%d\n", PEId);
  if (PEIntervals[PEId].size() == 0) {
    printf("None Interval\n");
    return;
  }
  for (int i = 0; i < PEIntervals[PEId].size(); ++ i) {
    printf("[%d, %d]\n", PEIntervals[PEId][i].StartTime, PEIntervals[PEId][i].EndTime);
  }
}

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
  assert(l <= r);
  if (l == r) {
    Cache[p][rt] = 0;
    return ;
  }
  Lazy[p][rt] = 0;
  int m = (l + r) >> 1;
  Build(p, l, m, rt << 1);
  Build(p, m + 1, r, rt << 1 | 1);
  PushUp(p, rt);
}

void Update(int p, int l, int r, int rt, int L, int R, int add) {
  assert(add > 0);
  assert(l <= r);
  if (L <= l && r <= R) {
    Cache[p][rt] = Cache[p][rt] + add;
    Lazy[p][rt] = Lazy[p][rt] + add;
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
  assert(l <= r);
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

void ShowCache(int p, int l, int r, int rt) {
  if (l == r) {
    printf("%d\t", Cache[p][rt]);
    return ;
  }
  PushDown(p, rt);
  int m = (l + r) >> 1;
  ShowCache(p, l, m, rt << 1);
  ShowCache(p, m + 1, r, rt << 1 | 1);
  PushUp(p, rt);
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
  memset(Checked, false, sizeof(Checked));
  memset(ReChecked, false, sizeof(ReChecked));

  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  int NeedPE = GetTopology();
  printf("Multi:%d\n", NeedPE);

  if (TotalPE >= NeedPE) {
    NgList.push_back(NodeGenerator(TotalNode, NeedPE, UpRound, NodeList));
    IterList.push_back(Iteration(NeedPE));
    if (TotalPE % NeedPE != 0) {
      NgList.push_back(NodeGenerator(TotalNode, TotalPE % NeedPE, UpRound, NodeList));
      IterList.push_back(Iteration(TotalPE % NeedPE));
    }
  }
  else {
    NgList.push_back(NodeGenerator(TotalNode, TotalPE, UpRound, NodeList));
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

vector<Node> GetKeyNodeSet(vector<Node> ChoosedNodes) {
  vector<Node> KeyNodeSet;
  for (int i = 0; i < ChoosedNodes.size(); ++ i) {
    if (ChoosedNodes[i].OutDegree == 0)
      KeyNodeSet.push_back(ChoosedNodes[i]);
  }

  sort(KeyNodeSet.begin(), KeyNodeSet.end(), CmpByCost);
  return KeyNodeSet;
}

void ArrangeKeyNode(Node KeyNode, NodeGenerator &ng, priority_queue<Node, vector<Node>, NodeComparationByCost> &Que) {
  // printf("ArrangeKeyNode\n");
  // KeyNode.Show();
  // arrange keynode position
  long long PeriodTime = ng.UpBound;

  vector<Edge> Attributes;
  int LCost = 0;
  int RCost = 0;
  bool Whole = false;
  // deal with in edge
  vector<Edge> InEdges = ReEdgeList[KeyNode.Id];
  for (int i = 0; i < InEdges.size(); ++ i) {
    Edge e = InEdges[i];
    if (e.CacheTimeCost >= PeriodTime)
      Whole = true;
    long long NLCost = min(KeyNode.StartTime,  1LL * e.CacheTimeCost);
    long long NRCost = (e.CacheTimeCost <= KeyNode.StartTime ? 0 : e.CacheTimeCost - KeyNode.StartTime) % PeriodTime;
    LCost = max(LCost, (int)NLCost);
    RCost = max(RCost, (int)NRCost);
    Attributes.push_back(InEdges[i]);
  }

  if (Attributes.size() > 0) {
    int H = 0;
    if (Whole == false) {
      if (LCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, KeyNode.StartTime - LCost + 1, KeyNode.StartTime));
      if (RCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, PeriodTime - RCost + 1, PeriodTime));
    }
    else {
      H = Query(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime);
    }
    
    int BinSize = CACHESIZE - H;
    vector<int> NodeSizes;
    for (int i = 0; i < Attributes.size(); ++ i)
      NodeSizes.push_back(Attributes[i].Memory);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);

    // printf("Put into Cache\n");
    // put into cache
    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      if (i != 0) assert(ArrangedSet[i] > ArrangedSet[i - 1]);
      int index = ArrangedSet[i];
      Edge e = Attributes[index];
      Node ArNode = ng.GetNode(e.From, KeyNode.Round);
      ArNode.Retiming = Floor(KeyNode.StartTime - e.CacheTimeCost - ArNode.EndTime, PeriodTime);

      long long Dis = KeyNode.StartTime + KeyNode.Retiming * PeriodTime - (ArNode.EndTime + ArNode.Retiming * PeriodTime);
      long long NLCost = min(1LL * KeyNode.StartTime, Dis);
      long long NRCost = (Dis <= KeyNode.StartTime ? 0 : Dis - KeyNode.StartTime) % PeriodTime;

      if (Dis >= PeriodTime + KeyNode.StartTime)
        Update(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime, (Dis - KeyNode.StartTime / PeriodTime) * e.Memory);
      if (NLCost > 0) 
        Update(KeyNode.PEId, 1, PeriodTime, 1, KeyNode.StartTime - NLCost + 1, KeyNode.StartTime, e.Memory);
      if (NRCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, PeriodTime - NRCost + 1, PeriodTime, e.Memory);

      ng.SetNode(ArNode);
      ng.RunOnCache = ng.RunOnCache + 1;
      if (!Checked[ArNode.Id][ArNode.Round]) {
        Que.push(ArNode);
        Checked[ArNode.Id][ArNode.Round] = true;
      }
      Attributes.erase(Attributes.begin() + index);
    }

    // put into DRAM
    for (int i = 0; i < Attributes.size(); ++ i) {
      Edge e = Attributes[i];
      Node ArNode = ng.GetNode(e.From, KeyNode.Round);
      ArNode.Retiming = Floor(KeyNode.StartTime - e.DRAMTimeCost - ArNode.EndTime, PeriodTime);

      ng.SetNode(ArNode);
      ng.RunOnDRAM = ng.RunOnDRAM + 1;
      if (!Checked[ArNode.Id][ArNode.Round]) {
        Que.push(ArNode);
        Checked[ArNode.Id][ArNode.Round] = true;
      }
    }
  }
}

void BFS(vector<Node> KeyNodeSet, NodeGenerator &ng) {
  // printf("BFS\n");
  priority_queue<Node, vector<Node>, NodeComparationByCost> q;
  for (int i = 0; i < KeyNodeSet.size(); ++ i) {
    Node KeyNode = KeyNodeSet[i];
    KeyNode.Retiming = 0;
    Checked[KeyNode.Id][KeyNode.Round] = true;
    q.push(KeyNode);
    ng.SetNode(KeyNode);
  }
  while (!q.empty()) {
    Node SourceNode = q.top();
    q.pop();
    Checked[SourceNode.Id][SourceNode.Round] = false;
    ArrangeKeyNode(ng.GetNode(SourceNode.Id, SourceNode.Round), ng, q);
  }
  // printf("BFS Succeed\n");
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
  vector<Node> KeyNodeSet = GetKeyNodeSet(UnCheckedNodes);
  BFS(KeyNodeSet, ng);
}

FinalResult CalcFinalResult(int TotalCost, int Launches, int PeriodTimes) {
  for (int i = 0; i < NgList.size(); ++ i) {
    printf("\nUpBound:%d\tUpRound:%d\n", NgList[i].UpBound, NgList[i].UpRound);
    assert(NgList[i].UpBound <= MAXM);
    memset(Checked, false, sizeof(Checked));
    for (int j = 1; j <= NgList[i].NeedPE; ++ j)
      Build(j, 1, NgList[i].UpBound, 1);

    // printf("Start Spread KeyNode Set\n");
    SpreadKeyNodeSet(NgList[i]);
    // printf("End Spread KeyNode Set\n");
    // NgList[i].Show();
    // NgList[i].ShowEach(false);
    // NgList[i].ShowRelation();
    NgList[i].CalcPrelogue();
  }

  FinalResult FR = FinalResult();
  if (NgList.size() == 1) {
    int Each = Ceil(PeriodTimes, Launches);
    int X = Ceil(Each - NgList[0].UpRound, NgList[0].UpRound);
    FR.TotalTime = NgList[0].Prelogue + 1LL * X * NgList[0].UpBound;
    FR.Prelogue = NgList[0].Prelogue;
    FR.Retiming = NgList[0].Retiming;
    FR.RunOnCache = NgList[0].RunOnCache * X * Launches;
    FR.RunOnDRAM = NgList[0].RunOnDRAM * X * Launches;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
      int EachY = PeriodTimes - EachX;
      int X = max(0, (int)Ceil(EachX - NgList[0].UpRound, NgList[0].UpRound));
      int Y = max(0, (int)Ceil(EachY - NgList[1].UpRound, NgList[1].UpRound));
      long long TotalTimeX = NgList[0].Prelogue + 1LL * X * NgList[0].UpBound;
      long long TotalTimeY = NgList[1].Prelogue + 1LL * Y * NgList[1].UpBound;
      long long TotalTime = max(TotalTimeX, TotalTimeY);
      if (FR.TotalTime == -1 || TotalTime < FR.TotalTime) {
        FR.TotalTime = TotalTime;
        FR.RunOnCache = NgList[0].RunOnCache * X * Launches + NgList[1].RunOnCache * Y;
        FR.RunOnDRAM = NgList[0].RunOnDRAM * X * Launches + NgList[1].RunOnDRAM * Y;
      }
    }
    FR.Prelogue = NgList[0].Prelogue;
    FR.Retiming = NgList[0].Retiming;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  return FR;
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