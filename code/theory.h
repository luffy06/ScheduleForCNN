struct Node {
  int Id;
  int Cost;

  int PEId;       // run on which pe
  int Round;
  int Retiming;

  int InDegree;
  int OutDegree;
  int TopoOrder;

  long long StartTime;
  long long EndTime;

  bool Certained;

  Node() {
    Round = PEId = -1;
    StartTime = EndTime = -1;
    Retiming = 0;
    Certained = false;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
  }

  Node(int a, int b) {
    Id = a;
    Cost = b;
    Round = PEId = -1;
    StartTime = EndTime = -1;
    Retiming = 0;
    Certained = false;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
  }

  void SetTime(long long st, long long ed) {
    assert(StartTime <= EndTime);
    StartTime = st;
    EndTime = ed;
  }

  void Copy(Node a) {
    Id = a.Id;
    Cost = a.Cost;

    InDegree = a.InDegree;
    OutDegree = a.OutDegree;
    TopoOrder = a.TopoOrder;

    PEId = a.PEId;
    Round = a.Round;
    Retiming = a.Retiming;
    StartTime = a.StartTime;
    EndTime = a.EndTime;

    Certained = a.Certained;
  }

  void Show() {
    printf("ID:%2d\tPE:%2d\tRound:%2d\tRetiming:%2d\tST:%lld\tED:%lld\tCost:%d\tStatus:%s\tTopoOrder:%d\n", Id, PEId, Round, Retiming, StartTime, EndTime, Cost, (Certained ? "Certained" : "Uncertained"), TopoOrder);
  }
};

Node NodeList[MAXN];

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
    assert(StartTime <= EndTime);
    StartTime = a;
    EndTime = b;
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
    if (Relation[L].ToId != ToId) {
      printf("GetRelatedNode\tFrom Id:%d\tTo Id:%d\tTo Round:%d\n", FromId, ToId, ToRound);
      ShowRelation();
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

typedef pair<Edge, bool> EdgeBool;

vector<NodeGenerator> NgList;
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
    if (TotalPE % NeedPE != 0)
      NgList.push_back(NodeGenerator(TotalNode, TotalPE % NeedPE, UpRound, NodeList));
  }
  else {
    NgList.push_back(NodeGenerator(TotalNode, TotalPE, UpRound, NodeList));
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
    printf("### Bad BinSize Of %d ###\n", BinSize);
    printf("Good Size:%lu\n", Goods.size());
    BinSize = Sum - BinSize;
    printf("BinSize:%d\tSum:%d\tMAXSIZE:%d\n", BinSize, Sum, MAXSIZE);
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

vector<Node> GetKeyNodeSet(vector<Node> &ChoosedNodes) {
  vector<Node> KeyNodeSet;
  for (int i = 0; i < ChoosedNodes.size();) {
    if (Checked[ChoosedNodes[i].Id][ChoosedNodes[i].Round])
      ChoosedNodes.erase(ChoosedNodes.begin() + i);
    else
      ++ i;
  }
  int MaxCost = -1;
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

Node FindClosestNode(int NodeId, int Cost, Node KeyNode, NodeGenerator &ng, bool &InQ) {
  long long PeriodTime = ng.UpBound;
  vector<Node> ChoosedNodes = ng.GetChoosedNode(NodeId, KeyNode.Id);
  assert(ChoosedNodes.size() > 0);
  
  Node ArNode = Node();
  int TargetInt = -1;
  // 选取最近的PE
  for (int j = 0; j < ChoosedNodes.size(); ++ j) {
    Node NowNode = ChoosedNodes[j];
    long long EndTime = KeyNode.StartTime + KeyNode.Retiming * PeriodTime - Cost;
    long long StartTime = EndTime - NowNode.Cost;
    
    int Int = -1;
    if (!NowNode.Certained) {
      int PEId = NowNode.PEId;
      
      for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
        if (PEIntervals[PEId][k].StartTime <= NowNode.StartTime 
          && PEIntervals[PEId][k].EndTime >= NowNode.EndTime) {
          Int = k;
          break;
        }
      }
      if (Int == -1) {
        NowNode.Show();
        ShowInterval(PEId);
      }
      assert(Int != -1);
      PEInterval interval = PEIntervals[PEId][Int];

      // PS + Retiming * P <= S
      NowNode.Retiming = Floor(StartTime - interval.StartTime, PeriodTime);
      EndTime = min(interval.EndTime + NowNode.Retiming * PeriodTime, EndTime);
      StartTime = EndTime - NodeList[NodeId].Cost;
      int BinSize = StartTime - (interval.StartTime + NowNode.Retiming * PeriodTime);
      
      vector<Node> SamePENodes = ng.GetSamePEOtherNodes(PEId, NodeId, ChoosedNodes[j].Round, 
                                                        interval.StartTime, 
                                                        interval.EndTime);
      vector<int> NodeSizes;
      for (int k = 0; k < SamePENodes.size(); ++ k)
        NodeSizes.push_back(SamePENodes[k].Cost);
      vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
      int Sum = 0;
      for (int k = 0; k < ArrangedSet.size(); ++ k)
        Sum = Sum + NodeSizes[ArrangedSet[k]];

      assert(Sum <= BinSize);
      NowNode.SetTime(interval.StartTime + Sum, interval.StartTime + Sum + NowNode.Cost);
    }
    else {
      // AS + Retiming * P <= S
      if (NowNode.StartTime + NowNode.Retiming * PeriodTime > StartTime)
        NowNode.Retiming = Floor(StartTime - NowNode.StartTime, PeriodTime);
    }
    assert(NowNode.StartTime + NowNode.Retiming * PeriodTime <= StartTime);

    if (ArNode.StartTime == -1 
      || NowNode.StartTime + NowNode.Retiming * PeriodTime > ArNode.StartTime + ArNode.Retiming * PeriodTime) {
      ArNode.Copy(NowNode);
      TargetInt = Int;
    }
  }

  InQ = false;
  if (!ArNode.Certained) {
    InQ = true;
    // printf("Update Other Nodes\n");
    // update other nodes' moving interval
    int BinSize = ArNode.StartTime - PEIntervals[ArNode.PEId][TargetInt].StartTime;  
    vector<Node> SamePENodes = ng.GetSamePEOtherNodes(ArNode.PEId, ArNode.Id, ArNode.Round, 
                                                      PEIntervals[ArNode.PEId][TargetInt].StartTime, 
                                                      PEIntervals[ArNode.PEId][TargetInt].EndTime);
    vector<int> NodeSizes;
    for (int k = 0; k < SamePENodes.size(); ++ k)
      NodeSizes.push_back(SamePENodes[k].Cost);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
    int Sum = 0;
    for (int k = 0; k < ArrangedSet.size(); ++ k)
      Sum = Sum + NodeSizes[ArrangedSet[k]];

    int StartTime = PEIntervals[ArNode.PEId][TargetInt].StartTime;
    int EndTime = ArNode.StartTime;
    // printf("Before:[%d, %d]\n", StartTime, EndTime);
    if (ArNode.PEId == 45)
      printf("%lu %lu\n", SamePENodes.size(), ArrangedSet.size());
    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      if (i != 0) assert(ArrangedSet[i] > ArrangedSet[i - 1]);
      Node NowNode = SamePENodes[ArrangedSet[i]];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNode(NowNode);
      SamePENodes.erase(SamePENodes.begin() + ArrangedSet[i]);
    }

    StartTime = ArNode.EndTime;
    EndTime = PEIntervals[ArNode.PEId][TargetInt].EndTime;
    // printf("After:[%d, %d]\n", StartTime, EndTime);
    for (int i = 0; i < SamePENodes.size(); ++ i) {
      Node NowNode = SamePENodes[i];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNode(NowNode);
    }
    ArNode.Certained = true;
    assert(TargetInt != -1);
    DeleteInterval(ArNode.PEId, TargetInt, ArNode.StartTime, ArNode.EndTime);
  }
  return ArNode;
}

void ArrangeKeyNode(Node KeyNode, NodeGenerator &ng, priority_queue<Node, vector<Node>, NodeComparationByCost> &Que) {
  // printf("ArrangeKeyNode\n");
  if (KeyNode.Id == 509)
    KeyNode.Show();
  // arrange keynode position
  long long PeriodTime = ng.UpBound;

  vector<Node> ReadyForCache;
  vector<EdgeBool> Attributes;
  int LCost = 0;
  int RCost = 0;
  bool Whole = false;
  // deal with in edge
  vector<Edge> InEdges = ReEdgeList[KeyNode.Id];
  sort(InEdges.begin(), InEdges.end(), CmpEdgeByFromCost);
  for (int i = 0; i < InEdges.size(); ++ i) {
    bool InQ = false;
    Node FromNode = FindClosestNode(InEdges[i].From, InEdges[i].CacheTimeCost, KeyNode, ng, InQ);
    if (FromNode.EndTime + FromNode.Retiming * PeriodTime + InEdges[i].DRAMTimeCost > KeyNode.StartTime + KeyNode.Retiming * PeriodTime) {
      // printf("Ready put into Cache\n");
      // ready put into cache
      ReadyForCache.push_back(FromNode);
      Attributes.push_back(make_pair(InEdges[i], InQ));

      // get query cache interval
      if ((KeyNode.StartTime + KeyNode.Retiming * PeriodTime) 
        - (FromNode.EndTime + FromNode.Retiming * PeriodTime) >= PeriodTime)
        Whole = true;
      long long Dis = KeyNode.StartTime + KeyNode.Retiming * PeriodTime - (FromNode.EndTime + FromNode.Retiming * PeriodTime);
      long long NLCost = min(KeyNode.StartTime, Dis);
      long long NRCost = (Dis <= KeyNode.StartTime ? 0 : Dis - KeyNode.StartTime) % PeriodTime;
      LCost = max(LCost, (int)NLCost);
      RCost = max(RCost, (int)NRCost);
    }
    else {
      // printf("Put into DRAM\n");
      // put into DRAM
      if (InQ)
        Que.push(FromNode);
      else
        ReChecked[FromNode.Id][FromNode.Round] = true;
      ng.SetNode(FromNode);
      ng.AddRelation(CertainedEdge(FromNode.Id, FromNode.Round, KeyNode.Id, KeyNode.Round, false));
      Checked[FromNode.Id][FromNode.Round] = true;
    }
  }

  if (ReadyForCache.size() > 0) {
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
      NodeSizes.push_back(Attributes[i].first.Memory);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);

    // printf("Put into Cache\n");
    // put into cache
    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      if (i != 0) assert(ArrangedSet[i] > ArrangedSet[i - 1]);
      int index = ArrangedSet[i];
      Node ArNode = ReadyForCache[index];
      bool InQ = Attributes[index].second;

      int Memory = Attributes[index].first.Memory;
      long long Dis = KeyNode.StartTime + KeyNode.Retiming * PeriodTime - (ArNode.EndTime + ArNode.Retiming * PeriodTime);
      long long NLCost = min(1LL * KeyNode.StartTime, Dis);
      long long NRCost = (Dis <= KeyNode.StartTime ? 0 : Dis - KeyNode.StartTime) % PeriodTime;

      if (Dis >= PeriodTime + KeyNode.StartTime)
        Update(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime, (Dis - KeyNode.StartTime / PeriodTime) * Memory);
      if (NLCost > 0) 
        Update(KeyNode.PEId, 1, PeriodTime, 1, KeyNode.StartTime - NLCost + 1, KeyNode.StartTime, Memory);
      if (NRCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, PeriodTime - NRCost + 1, PeriodTime, Memory);

      if (InQ)
        Que.push(ArNode);
      else
        ReChecked[ArNode.Id][ArNode.Round] = true;
      ng.SetNode(ArNode);
      ng.AddRelation(CertainedEdge(ArNode.Id, ArNode.Round, KeyNode.Id, KeyNode.Round, true));
      Checked[ArNode.Id][ArNode.Round] = true;

      ReadyForCache.erase(ReadyForCache.begin() + index);
      Attributes.erase(Attributes.begin() + index);
    }

    // put into DRAM
    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      Node ArNode = ReadyForCache[i];
      Edge e = Attributes[i].first;
      bool InQ = Attributes[i].second;
      long long EndTime = KeyNode.StartTime + KeyNode.Retiming * PeriodTime - e.DRAMTimeCost;
      long long StartTime = EndTime - ArNode.Cost;
      if (ArNode.StartTime + ArNode.Retiming * PeriodTime > StartTime)
        ArNode.Retiming = Floor(StartTime - ArNode.StartTime, PeriodTime);

      if (InQ)
        Que.push(ArNode);
      else
        ReChecked[ArNode.Id][ArNode.Round] = true;
      ng.SetNode(ArNode);
      ng.AddRelation(CertainedEdge(ArNode.Id, ArNode.Round, KeyNode.Id, KeyNode.Round, false));
      Checked[ArNode.Id][ArNode.Round] = true;
    }
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

void ReBFS(Node KeyNode, NodeGenerator &ng) {
  // printf("ReBFS\n");
  long long PeriodTime = ng.UpBound;
  ng.SortRealtion();
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
      // printf("From:%d To:%d %d %d\n", InEdges[i].From, InEdges[i].To, ToNode.Id, ToNode.Round);
      bool StrogeInCache = false;
      int FromRound = ng.GetRelatedNode(InEdges[i].From, ToNode.Id, ToNode.Round, StrogeInCache);
      if (FromRound == -1) {
        printf("%d %d %d\n", InEdges[i].From, ToNode.Id, ToNode.Round);
        ng.ShowRelation();
      }
      assert(FromRound != -1);
      Node FromNode = ng.GetNode(InEdges[i].From, FromRound);
      long long Cost = (StrogeInCache ? InEdges[i].CacheTimeCost : InEdges[i].DRAMTimeCost);
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

    // printf("Get KeyNode Set\n");
    // for (int i = 0; i < KeyNodeSet.size(); ++ i) {
    //   KeyNodeSet[i].Show();
    // }

    for (int i = 0; i < KeyNodeSet.size(); ++ i) {
      Node KeyNode = KeyNodeSet[i];
      KeyNode = ng.GetNode(KeyNode.Id, KeyNode.Round);
      if (!Checked[KeyNode.Id][KeyNode.Round]) {
        PlaceKeyNode(KeyNode, ng);
        BFS(KeyNode, ng);
      }
    }
  } while (!UnCheckedNodes.empty());

  // ng.Show();
  // ng.ShowEach(false);
  // ng.ShowRelation();

  printf("Spread Succeed\n");

  vector<Node> ReCheckedNodes;
  for (int i = 1; i <= TotalNode; ++ i)
    for (int j = 1; j <= ng.UpRound; ++ j)
      if (ReChecked[i][j]) ReCheckedNodes.push_back(ng.GetNode(i, j));
  
  sort(ReCheckedNodes.begin(), ReCheckedNodes.end(), CmpByTopoOrder);
  printf("ReChecked Nodes\n");
  for (int i = ReCheckedNodes.size() - 1; i >= 0; -- i) {
    Node KeyNode = ReCheckedNodes[i];
    if (ReChecked[KeyNode.Id][KeyNode.Round])
      ReBFS(ng.GetNode(KeyNode.Id, KeyNode.Round), ng);
  }
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

  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;
  int Launches = TotalPE / NgList[0].NeedPE;

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
        FR.RunOnDRAM = NgList[0].RunOnDRAM * X * Launches + NgList[1].RunOnCache * Y;
      }
    }
    FR.Prelogue = NgList[0].Prelogue;
    FR.Retiming = NgList[0].Retiming;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  return FR;
}