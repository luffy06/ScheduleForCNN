struct Node {
  int Id;
  int Cost;

  int PEId;       // run on which pe
  int Round;
  int Retiming;
  int Color;

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

    PEId = a.PEId;
    Round = a.Round;
    Retiming = a.Retiming;
    StartTime = a.StartTime;
    EndTime = a.EndTime;
  }

  void Show() {
    printf("ID:%d\tPE:%d\tRound:%d\tRetiming:%d\tST:%lld\tED:%lld\tCost:%d\tStatus:%s\n", Id, PEId, Round, Retiming, StartTime, EndTime, Cost, (Certained ? "Certained" : "Uncertained"));
  }
};

Node NodeList[MAXN];

typedef pair<Node, int> NodeInt;
typedef pair<NodeInt, int> NodeTwoInt;

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

  friend bool operator< (CertainedEdge a, CertainedEdge b) {
    if (a.FromId != b.FromId)
      return a.FromId > b.FromId;
    else if (a.FromRound != b.FromRound)
      return a.FromRound > b.FromRound;
    else if (a.ToId != b.ToId)
      return a.ToId > b.ToId;
    return a.ToRound > b.ToRound;
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
  else if (a.StartTime != b.StartTime)
    return a.StartTime < b.StartTime;
  else if (a.EndTime != b.EndTime)
    return a.EndTime < b.EndTime;
  else if (MapTopology[a.Id] != MapTopology[b.Id])
    return MapTopology[a.Id] < MapTopology[b.Id];
  else if (a.Round != b.Round)
    return a.Round < b.Round;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
}

bool CmpFourInt(FourInt a, FourInt b) {
  if (a.first.first != b.first.first)
    return a.first.first < b.first.first;
  else if (a.first.second != b.first.second)
    return a.first.second < b.first.second;
  else if (a.second.first != b.second.first)
    return a.second.first < b.second.first;
  return a.second.second < b.second.second;
}

bool CmpEdgeByFromCost(Edge a, Edge b) {
  if (NodeList[a.From].Cost != NodeList[b.From].Cost)
    return NodeList[a.From].Cost > NodeList[b.From].Cost;
  return a.From < b.From;
}

bool CmpEdgeByToCost(Edge a, Edge b) {
  if (NodeList[a.To].Cost != NodeList[b.To].Cost)
    return NodeList[a.To].Cost > NodeList[b.To].Cost;
  return a.To < b.To;
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
    assert(StartTime <= EndTime);
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
  long long Prelogue;
  int Retiming;
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
        UpBound = max(UpBound, (int)(Emp.EndTime + n.Cost));
        StartTable.push_back(n);
      }
    }
    sort(NodeList + 1, NodeList + TotalNode + 1, CmpById);
    // calculate the use ratio of cpu
    assert(UpBound != 0);
    double Down = UpBound * NeedPE;
    double Up = 0;
    for (int i = 1; i <= TotalNode; ++ i) {
      Up = Up + NodeList[i].Cost;
    }
    Up = Up * UpRound;
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

  vector<int> GetNodeInRounds(int NodeId, int Condition, bool Checked[MAXN][MAXR]) {
    vector<int> Rounds;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      if (StartTable[i].Id == NodeId) {
        if (Condition == 1 && !Checked[NodeId][StartTable[i].Round])
          Rounds.push_back(StartTable[i].Round);
        else if (Condition == 2 && !StartTable[i].Certained)
          Rounds.push_back(StartTable[i].Round);
      }
    }
    return Rounds;
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

  void SetNodeTime(Node ArNode) {
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

  Node GetNodeByColor(int NodeId, int Color) {
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i)
      if (StartTable[i].Color == Color)
        return StartTable[i];
    printf("### Not Found Node:Id:%d\tColor:%d ###\n", NodeId, Color);
    assert(1 == 0);
    return Node();    
  }

  bool HasUncertainedNodes() {
    for (int i = 0; i < StartTable.size(); ++ i)
      if (!StartTable[i].Certained)
        return true;
    return false;
  }

  void CalcPrelogue() {
    int MinRetiming = INF;
    int MaxRetiming = -1;
    for (int i = 0; i < StartTable.size(); ++ i) {
      MinRetiming = min(MinRetiming, StartTable[i].Retiming);
      MaxRetiming = max(MaxRetiming, StartTable[i].Retiming);
    }
    Retiming = MaxRetiming - MinRetiming + 1;
    Prelogue = 1LL * Retiming * UpBound;
  }

  vector<Node> GetUncertainedNodes() {
    vector<Node> UncertainedNodes;
    for (int i = 0; i < StartTable.size(); ++ i)
      if (!StartTable[i].Certained)
        UncertainedNodes.push_back(StartTable[i]);

    return UncertainedNodes;
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
        printf("%c", c);
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

vector<NodeGenerator> NgList;
int Topology[MAXN];
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;
vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
int DP[MAXN][MAXSIZE];
vector<PEInterval> PEIntervals[MAXPE];
bool Checked[MAXN][MAXR];
int Cache[MAXPE][MAXM << 2];
int Lazy[MAXPE][MAXM << 2];
vector<TwoInt> Visit;
vector<CertainedEdge> CertainedEdges;
int RunOnCache;
int RunOnDRAM;

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
  int Count = 0, Iter = 0, Order = 0;
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
  return NeedPE;
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

  RunOnDRAM = RunOnCache = 0;
  int NeedPE = GetTopology();
  printf("Multi:%d\n", NeedPE);

  if (TotalPE >= NeedPE) {
    NgList.push_back(NodeGenerator(TotalNode, NeedPE, UpRound, NodeList));
    if (TotalPE % NeedPE != 0) {
      NgList.push_back(NodeGenerator(TotalNode, TotalPE % NeedPE, UpRound, NodeList));
    }
  }
  else {
    NgList.push_back(NodeGenerator(TotalNode, TotalPE, UpRound, NodeList));
  }
}

vector<Node> GetKeyNodeSet() {
  vector<Node> KeyNodeSet;
  int MaxCost = -1;
  for (int i = 1; i <= TotalNode; ++ i)
    MaxCost = max(MaxCost, NodeList[i].Cost);

  for (int i = 1; i <= TotalNode; ++ i) {
    if (NodeList[i].Cost >= (int)(MaxCost * ALPHA))
      KeyNodeSet.push_back(NodeList[i]);
  }

  sort(KeyNodeSet.begin(), KeyNodeSet.end(), CmpByCost);
  return KeyNodeSet;
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
   
/*
* MAXROUND * TotalNode
*/
bool ArrangeConnectedNode(Edge e, Node KeyNode, int Direction, NodeGenerator &ng, Node &ArNode, int &Int, int Condition) {
  assert(Direction == 1 || Direction == -1);
  int NodeId = (Direction == -1 ? e.From : e.To);
  vector<int> Rounds = ng.GetNodeInRounds(NodeId, Condition, Checked);
  if (Condition == 1 && Rounds.size() == 0)
    return false;

  bool PlaceInCache = true;

  int TargetInt = -1;
  int PeriodTime = ng.UpBound;
  // 选取最近的PE
  for (int j = 0; j < Rounds.size(); ++ j) {
    Node NowNode = ng.GetNode(NodeId, Rounds[j]);
    int Retiming = 0;
    int PEId = NowNode.PEId;

    long long StartTime = (Direction == -1 ? KeyNode.StartTime - e.CacheTimeCost - NodeList[NodeId].Cost : KeyNode.EndTime + e.CacheTimeCost);
    long long EndTime = StartTime + NodeList[NodeId].Cost;
    
    if (StartTime < 0) {
      Retiming = -Ceil(-StartTime, PeriodTime);
    }
    else if (EndTime >= PeriodTime) {
      Retiming = EndTime / PeriodTime;
    }

    Int = -1;
    for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
      if (PEIntervals[PEId][k].StartTime + Retiming * PeriodTime <= StartTime 
        && PEIntervals[PEId][k].EndTime + Retiming * PeriodTime >= EndTime) {
        Int = k;
        break;
      }
    }
    if (Int == -1) {
      bool Found = false;
      for (int i = 0; i < 2; ++ i) {
        if (Direction == -1) {
          for (int k = PEIntervals[PEId].size() - 1; k >= 0 ; -- k) {
            if (PEIntervals[PEId][k].EndTime - PEIntervals[PEId][k].StartTime >= NodeList[NodeId].Cost
              && PEIntervals[PEId][k].StartTime + (Retiming - i) * PeriodTime <= StartTime) {
                Retiming = Retiming - i;
                Int = k;
                Found = true;
                break;
            }
          }
        }
        else {
          for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
            if (PEIntervals[PEId][k].EndTime - PEIntervals[PEId][k].StartTime >= NodeList[NodeId].Cost
             && PEIntervals[PEId][k].EndTime + (Retiming + i) * PeriodTime >= EndTime) {
              Retiming = Retiming + i;
              Int = k;
              Found = true;
              break;
            }
          }
        }
        if (Found)
          break;
      }
    }

    if (Int == -1) {
      printf("### Not Found Interval ###\n");
      printf("Id:%d\tRound:%d\tPEId:%d\tRetiming:%d\tCost:%d\n", NodeId, Rounds[j], PEId, Retiming, NodeList[NodeId].Cost);
      printf("ST:%lld\tED:%lld\n", StartTime, EndTime);
      printf("PeriodTime:%d\n", PeriodTime);
      printf("%d\n", ng.UpRound);
    }
    assert(Int != -1);

    if (Direction == -1) {
      EndTime = min(PEIntervals[PEId][Int].EndTime + 1LL * Retiming * PeriodTime, EndTime);
      StartTime = EndTime - NodeList[NodeId].Cost;
    }
    else {
      StartTime = max(PEIntervals[PEId][Int].StartTime + 1LL * Retiming * PeriodTime, StartTime);
      EndTime = StartTime + NodeList[NodeId].Cost;
    }
    
    int BinSize = 0;
    if (Direction == -1)
      BinSize = StartTime - (PEIntervals[PEId][Int].StartTime + Retiming * PeriodTime);
    else
      BinSize = (PEIntervals[PEId][Int].EndTime + Retiming * PeriodTime) - EndTime;
    
    vector<Node> SamePENodes = ng.GetSamePEOtherNodes(PEId, NodeId, Rounds[j], PEIntervals[PEId][Int].StartTime, PEIntervals[PEId][Int].EndTime);
    vector<int> NodeSizes;
    for (int k = 0; k < SamePENodes.size(); ++ k)
      NodeSizes.push_back(SamePENodes[k].Cost);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
  
    int Sum = 0;
    for (int k = 0; k < ArrangedSet.size(); ++ k)
      Sum = Sum + NodeSizes[ArrangedSet[k]];

    assert(Sum <= BinSize);
    if (Direction == -1) {
      StartTime = PEIntervals[PEId][Int].StartTime + Retiming * PeriodTime + Sum;
      EndTime = StartTime + NodeList[NodeId].Cost;
    }
    else {
      EndTime = PEIntervals[PEId][Int].EndTime + Retiming * PeriodTime - Sum;
      StartTime = EndTime - NodeList[NodeId].Cost;
    }

    if (Direction == -1) {
      if (ArNode.StartTime == -1 || StartTime > ArNode.StartTime) {
        ArNode.Copy(NowNode);
        ArNode.SetTime(StartTime, EndTime);
        ArNode.Retiming = Retiming;
        TargetInt = Int;
      }
    }      
    else {
      if (ArNode.StartTime == -1 || StartTime < ArNode.StartTime) {
        ArNode.Copy(NowNode);
        ArNode.SetTime(StartTime, EndTime);
        ArNode.Retiming = Retiming;
        TargetInt = Int;
      }
      else if (StartTime == ArNode.StartTime && PEId == KeyNode.PEId) {
        ArNode.Copy(NowNode);
        ArNode.SetTime(StartTime, EndTime);
        ArNode.Retiming = Retiming;
        TargetInt = Int;
      }
    }
  }

  int BinSize = 0;
  if (Direction == -1)
    BinSize = ArNode.StartTime - (PEIntervals[ArNode.PEId][TargetInt].StartTime + ArNode.Retiming * PeriodTime);
  else
    BinSize = PEIntervals[ArNode.PEId][TargetInt].EndTime + ArNode.Retiming * PeriodTime - ArNode.EndTime;
  vector<Node> SamePENodes = ng.GetSamePEOtherNodes(ArNode.PEId, ArNode.Id, ArNode.Round, PEIntervals[ArNode.PEId][TargetInt].StartTime, PEIntervals[ArNode.PEId][TargetInt].EndTime);

  vector<int> NodeSizes;
  for (int k = 0; k < SamePENodes.size(); ++ k)
    NodeSizes.push_back(SamePENodes[k].Cost);
  vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);

  int Sum = 0;
  for (int k = 0; k < ArrangedSet.size(); ++ k)
    Sum = Sum + NodeSizes[ArrangedSet[k]];

  if (Direction == -1) {
    long long StartTime = PEIntervals[ArNode.PEId][TargetInt].StartTime;
    long long EndTime = ArNode.StartTime - ArNode.Retiming * PeriodTime;
    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      Node NowNode = SamePENodes[ArrangedSet[i]];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNodeTime(NowNode);
      SamePENodes.erase(SamePENodes.begin() + ArrangedSet[i]);
    }
    StartTime = ArNode.EndTime - ArNode.Retiming * PeriodTime;
    EndTime = PEIntervals[ArNode.PEId][TargetInt].EndTime;
    for (int i = 0; i < SamePENodes.size(); ++ i) {
      Node NowNode = SamePENodes[i];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNodeTime(NowNode);
    }
  }
  else {
    long long StartTime = ArNode.EndTime - ArNode.Retiming * PeriodTime;
    long long EndTime = PEIntervals[ArNode.PEId][TargetInt].EndTime;
    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      Node NowNode = SamePENodes[ArrangedSet[i]];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNodeTime(NowNode);
      SamePENodes.erase(SamePENodes.begin() + ArrangedSet[i]);
    }
    StartTime = PEIntervals[ArNode.PEId][TargetInt].StartTime;
    EndTime = ArNode.StartTime - ArNode.Retiming * PeriodTime;
    for (int i = 0; i < SamePENodes.size(); ++ i) {
      Node NowNode = SamePENodes[i];
      NowNode.SetTime(StartTime, EndTime);
      ng.SetNodeTime(NowNode);
    }
  }

  assert(ArNode.Round != -1);
  if (Direction == -1)
    PlaceInCache = KeyNode.StartTime - ArNode.EndTime < e.DRAMTimeCost;
  else
    PlaceInCache = ArNode.StartTime - KeyNode.EndTime < e.DRAMTimeCost;

  ArNode.SetTime(ArNode.StartTime - ArNode.Retiming * PeriodTime, ArNode.EndTime - ArNode.Retiming * PeriodTime);
  if (Condition == 1 && Direction == 1 && ArNode.PEId != KeyNode.PEId)
    return false;
  Checked[NodeId][ArNode.Round] = true;
  return PlaceInCache;
}

void DeleteInterval(int PEId, int Int, int StartTime, int EndTime) {
  if (PEIntervals[PEId][Int].StartTime < StartTime)
    PEIntervals[PEId].push_back(PEInterval(PEId, PEIntervals[PEId][Int].StartTime, StartTime));
  if (EndTime < PEIntervals[PEId][Int].EndTime)
    PEIntervals[PEId].push_back(PEInterval(PEId, EndTime, PEIntervals[PEId][Int].EndTime));
  PEIntervals[PEId].erase(PEIntervals[PEId].begin() + Int);
  sort(PEIntervals[PEId].begin(), PEIntervals[PEId].end());
}

queue<TwoInt> ArrangeKeyNode(Node KeyNode, NodeGenerator &ng) {
  // printf("Arrange KeyNode:");
  assert(KeyNode.PEId > 0 && KeyNode.PEId <= ng.NeedPE);
  Visit.push_back(make_pair(KeyNode.Id, KeyNode.Round));
  queue<TwoInt> CertainedNodes;
  // arrange keynode position
  bool Placed = KeyNode.Certained;
  int PeriodTime = ng.UpBound;

  if (KeyNode.Certained == false) {
    for (int i = 0; i < PEIntervals[KeyNode.PEId].size(); ) {
      if (PEIntervals[KeyNode.PEId][i].EndTime - PEIntervals[KeyNode.PEId][i].StartTime >= KeyNode.Cost) {
        Checked[KeyNode.Id][KeyNode.Round] = true;
        KeyNode.StartTime = PEIntervals[KeyNode.PEId][i].StartTime;
        KeyNode.EndTime = KeyNode.StartTime + KeyNode.Cost;
        KeyNode.Retiming = 0;
        KeyNode.Certained = true;
        assert(KeyNode.StartTime >= 0 && KeyNode.EndTime <= PeriodTime);
        ng.SetNodeTime(KeyNode);

        vector<Node> OtherNodes = ng.GetSamePEOtherNodes(KeyNode.PEId, KeyNode.Id, KeyNode.Round, 
                                                        PEIntervals[KeyNode.PEId][i].StartTime, PEIntervals[KeyNode.PEId][i].EndTime);
        for (int k = 0; k < OtherNodes.size(); ++ k) {
          Node OtherNode = OtherNodes[k];
          OtherNode.SetTime(KeyNode.EndTime, PEIntervals[KeyNode.PEId][i].EndTime);
          ng.SetNodeTime(OtherNode);
        }

        DeleteInterval(KeyNode.PEId, i, KeyNode.StartTime, KeyNode.EndTime);
        Placed = true;
        break;
      }
      else {
        ++ i;
      }
    }
  }
  assert(Placed == true); 

  vector<Node> ReadyForCache;
  vector<TwoInt> Attributes;
  int LCost = 0;
  int RCost = 0;
  bool Whole = false;
  // deal with in edge
  vector<Edge> InEdges = ReEdgeList[KeyNode.Id];
  sort(InEdges.begin(), InEdges.end(), CmpEdgeByFromCost);
  for (int i = 0; i < InEdges.size(); ++ i) {
    // printf("Arrange In-Edge:From:%d\tTo:%d\n", InEdges[i].From, KeyNode.Id);
    Node FromNode = Node();
    int Int = -1;
    if (ArrangeConnectedNode(InEdges[i], KeyNode, -1, ng, FromNode, Int, 1)) {
      if (Int == -1)
        continue;
      ReadyForCache.push_back(FromNode);
      Attributes.push_back(make_pair(Int, InEdges[i].Memory));
      if (KeyNode.StartTime - (FromNode.EndTime + 1LL * FromNode.Retiming * PeriodTime) >= PeriodTime)
        Whole = true;

      long long Dis = KeyNode.StartTime - (FromNode.EndTime + 1LL * FromNode.Retiming * PeriodTime);
      long long NLCost = min(KeyNode.StartTime, Dis);
      long long NRCost = (Dis <= KeyNode.StartTime ? 0 : Dis - KeyNode.StartTime);
      while (NRCost >= PeriodTime)
        NRCost = NRCost - PeriodTime;
      LCost = max(LCost, (int)NLCost);
      RCost = max(RCost, (int)NRCost);
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
      NodeSizes.push_back(Attributes[i].second);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);

    RunOnCache = RunOnCache + ArrangedSet.size();
    RunOnDRAM = RunOnDRAM + ReadyForCache.size() - ArrangedSet.size();

    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      Node ArNode = ReadyForCache[ArrangedSet[i]];
      int TargetInt = Attributes[ArrangedSet[i]].first;
      CertainedNodes.push(make_pair(ArNode.Id, ArNode.Round));

      DeleteInterval(ArNode.PEId, TargetInt, ArNode.StartTime, ArNode.EndTime);

      ArNode.Certained = true;
      ArNode.Color = KeyNode.Color;
      ng.SetNodeTime(ArNode);
      Checked[ArNode.Id][ArNode.Round] = true;
      CertainedEdges.push_back(CertainedEdge(ArNode.Id, ArNode.Round, KeyNode.Id, KeyNode.Round, true));

      int Memory = Attributes[ArrangedSet[i]].second;
      long long Dis = KeyNode.StartTime - (ArNode.EndTime + ArNode.Retiming * PeriodTime);
      long long NLCost = min(1LL * KeyNode.StartTime, Dis);
      long long NRCost = (Dis <= KeyNode.StartTime ? 0 : Dis - KeyNode.StartTime);
      while (NRCost >= PeriodTime)
        NRCost = NRCost - PeriodTime;

      if (KeyNode.StartTime - (ArNode.EndTime + ArNode.Retiming * PeriodTime) >= PeriodTime ) {
        for (int j = ArNode.EndTime; j <= KeyNode.StartTime; j = j + PeriodTime)
          Update(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime, Memory);
      }
      if (NLCost > 0) 
        Update(KeyNode.PEId, 1, PeriodTime, 1, KeyNode.StartTime - NLCost + 1, KeyNode.StartTime, Memory);
      if (NRCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, PeriodTime - NRCost + 1, PeriodTime, Memory);

      ReadyForCache.erase(ReadyForCache.begin() + ArrangedSet[i]);
      Attributes.erase(Attributes.begin() + ArrangedSet[i]);
    }
    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      Node ArNode = ReadyForCache[i];
      Checked[ArNode.Id][ArNode.Round] = true;
      
      ArNode.Certained = false;
      ng.SetNodeTime(ArNode);
    }
  }

  // deal with out edge whose pe's id is same
  ReadyForCache.clear();
  Attributes.clear();
  LCost = 0;
  RCost = 0;
  Whole = false;
  vector<Edge> OutEdges = EdgeList[KeyNode.Id];
  sort(OutEdges.begin(), OutEdges.end(), CmpEdgeByToCost);
  for (int i = 0; i < OutEdges.size(); ++ i) {
    // printf("Arrange Out-Edge:From:%d\tTo:%d\n", KeyNode.Id, OutEdges[i].To);
    Node ToNode = Node();
    int Int = -1;
    if (ArrangeConnectedNode(OutEdges[i], KeyNode, 1, ng, ToNode, Int, 1)) {
      if (Int == -1)
        continue;
      ReadyForCache.push_back(ToNode);
      Attributes.push_back(make_pair(Int, OutEdges[i].Memory));
      if (ToNode.StartTime + ToNode.Retiming * PeriodTime - KeyNode.EndTime >= PeriodTime)
        Whole = true;

      long long Dis = ToNode.StartTime + ToNode.Retiming * PeriodTime - KeyNode.EndTime;
      long long NRCost = min(1LL * (PeriodTime - KeyNode.EndTime), Dis);
      long long NLCost = (Dis <= PeriodTime - KeyNode.EndTime ? 0 : Dis - PeriodTime + KeyNode.EndTime);
      while (NLCost >= PeriodTime)
        NLCost = NLCost - PeriodTime;
      LCost = max(LCost, (int)NLCost);
      RCost = max(RCost, (int)NRCost);
    }
  }

  if (ReadyForCache.size() > 0) {
    int H = 0;
    if (Whole == false) {
      if (LCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, 1, LCost));
      if (RCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, KeyNode.EndTime + 1, KeyNode.EndTime + RCost));
    }
    else {
      H = Query(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime);
    }
    
    int BinSize = CACHESIZE - H;
    
    vector<int> NodeSizes;
    for (int i = 0; i < Attributes.size(); ++ i)
      NodeSizes.push_back(Attributes[i].second);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
    
    RunOnCache = RunOnCache + ArrangedSet.size();
    RunOnDRAM = RunOnDRAM + ReadyForCache.size() - ArrangedSet.size();

    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      Node ArNode = ReadyForCache[ArrangedSet[i]];
      int TargetInt = Attributes[ArrangedSet[i]].first;
      CertainedNodes.push(make_pair(ArNode.Id, ArNode.Round));

      DeleteInterval(ArNode.PEId, TargetInt, ArNode.StartTime, ArNode.EndTime);

      ArNode.Certained = true;
      ArNode.Color = KeyNode.Color;
      ng.SetNodeTime(ArNode);
      Checked[ArNode.Id][ArNode.Round] = true;
      CertainedEdges.push_back(CertainedEdge(KeyNode.Id, KeyNode.Round, ArNode.Id, ArNode.Round, true));

      int Memory = Attributes[ArrangedSet[i]].second;
      long long Dis = ArNode.StartTime + ArNode.Retiming * PeriodTime - KeyNode.EndTime;
      long long NRCost = min(1LL * (PeriodTime - KeyNode.EndTime), Dis);
      long long NLCost = (Dis <= PeriodTime - KeyNode.EndTime ? 0 : Dis - PeriodTime + KeyNode.EndTime);
      while (NLCost >= PeriodTime)
        NLCost = NLCost - PeriodTime;

      if (ArNode.StartTime + ArNode.Retiming * PeriodTime - KeyNode.EndTime >= PeriodTime) {
        for (int j = ArNode.StartTime; j >= KeyNode.EndTime; j = j - PeriodTime)
          Update(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime, Memory);
      }
      if (NLCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, 1, NLCost, Memory);
      if (NRCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, KeyNode.EndTime + 1, KeyNode.EndTime + NRCost, Memory);
      ReadyForCache.erase(ReadyForCache.begin() + ArrangedSet[i]);
      Attributes.erase(Attributes.begin() + ArrangedSet[i]);
    }

    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      Node ArNode = ReadyForCache[i];
      Checked[ArNode.Id][ArNode.Round] = true;

      ArNode.Certained = false;
      ng.SetNodeTime(ArNode);
    }
  }
  return CertainedNodes;
}

void SpreadKeyNodeSet(vector<Node> KeyNodeSet, NodeGenerator &ng) {
  queue<TwoInt> CertainedNodes;
  for (int i = 1; i <= ng.NeedPE; ++ i) {
    PEIntervals[i].clear();
    PEIntervals[i].push_back(PEInterval(i, 0, ng.UpBound));
  }
  for (int i = 0; i < KeyNodeSet.size(); ++ i) {
    vector<int> Rounds = ng.GetNodeInRounds(KeyNodeSet[i].Id, 1, Checked);
    for (int j = 0; j < Rounds.size(); ++ j) {
      Node KeyNode = ng.GetNode(KeyNodeSet[i].Id, Rounds[j]);
      if (i == 0) KeyNode.Color = j + 1;
      queue<TwoInt> Nodes = ArrangeKeyNode(KeyNode, ng);
      while (!Nodes.empty()) {
        CertainedNodes.push(Nodes.front());
        Nodes.pop();
      }
    }
  }
  printf("KeyNodeSet Finished\n");
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
    if (!CertainedNodes.empty())
      printf("Deal Certained Nodes:%lu\n",CertainedNodes.size());
    while (!CertainedNodes.empty()) {
      TwoInt f = CertainedNodes.front();
      CertainedNodes.pop();

      Node KeyNode = ng.GetNode(f.first, f.second);
      queue<TwoInt> Nodes = ArrangeKeyNode(KeyNode, ng);
      while (!Nodes.empty()) {
        CertainedNodes.push(Nodes.front());
        Nodes.pop();
      }
    }

    Node KeyNode = q.top();
    q.pop();
    if (Checked[KeyNode.Id][KeyNode.Round])
      continue;
    CertainedNodes = ArrangeKeyNode(KeyNode, ng);
  }
}

bool CheckIfUncertain(int NodeId, vector<Node> UncertainedNodes) {
  for (int i = 0; i < UncertainedNodes.size(); ++ i) 
    if (UncertainedNodes[i].Id == NodeId)
      return true;
  return false;
}

void AscertainNodes(NodeGenerator &ng) {
  if (!ng.HasUncertainedNodes())
    return ;
  vector<Node> UncertainedNodes = ng.GetUncertainedNodes();

  int PeriodTime = ng.UpBound;
  for (int i = 0; i < Visit.size(); ++ i) {
    printf("%d/%lu\n", i + 1, Visit.size());
    TwoInt v = Visit[i];
    Node KeyNode = ng.GetNode(v.first, v.second);

    vector<Edge> InEdges = ReEdgeList[KeyNode.Id];
    sort(InEdges.begin(), InEdges.end(), CmpEdgeByFromCost);
    for (int j = 0; j < InEdges.size(); ++ j) {
      if (!CheckIfUncertain(InEdges[j].From, UncertainedNodes))
        continue;
      Node ArNode = Node();
      int TargetInt = -1;
      ArrangeConnectedNode(InEdges[j], KeyNode, -1, ng, ArNode, TargetInt, 2);
      assert(TargetInt != -1);

      DeleteInterval(ArNode.PEId, TargetInt, ArNode.StartTime, ArNode.EndTime);
      
      ArNode.Certained = true;
      ArNode.Color = KeyNode.Color;
      ng.SetNodeTime(ArNode);
      CertainedEdges.push_back(CertainedEdge(ArNode.Id, ArNode.Round, KeyNode.Id, KeyNode.Round, false));
    
      bool Found = false;
      for (int k = 0; k < UncertainedNodes.size(); ++ k) {
        if (UncertainedNodes[k].Id == ArNode.Id && UncertainedNodes[k].Round == ArNode.Round) {
          UncertainedNodes.erase(UncertainedNodes.begin() + k);
          Found = true;
          break;
        }
      }
      assert(Found == true);
    }

    vector<Edge> OutEdges = EdgeList[KeyNode.Id];
    sort(OutEdges.begin(), OutEdges.end(), CmpEdgeByToCost);
    for (int j = 0; j < OutEdges.size(); j++) {
      if (!CheckIfUncertain(OutEdges[j].To, UncertainedNodes))
        continue;
      Node ArNode = Node();
      int TargetInt = -1;
      ArrangeConnectedNode(OutEdges[j], KeyNode,1, ng, ArNode, TargetInt, 2);
      assert(TargetInt != -1);

      DeleteInterval(ArNode.PEId, TargetInt, ArNode.StartTime, ArNode.EndTime);

      ArNode.Certained = true;
      ArNode.Color = KeyNode.Color;
      ng.SetNodeTime(ArNode);
      CertainedEdges.push_back(CertainedEdge(KeyNode.Id, KeyNode.Round, ArNode.Id, ArNode.Round, false));

      bool Found = false;
      for (int k = 0; k < UncertainedNodes.size(); ++ k) {
        if (UncertainedNodes[k].Id == ArNode.Id && UncertainedNodes[k].Round == ArNode.Round) {
          UncertainedNodes.erase(UncertainedNodes.begin() + k);
          Found = true;
          break;
        }
      }
      assert(Found == true);
    }
  }
  assert(ng.HasUncertainedNodes() == false);
}

int CheckEdge(int FromId, int FromRound, int ToId, bool &StrogeInCache) {
  int L = 0;
  int R = CertainedEdges.size() - 1;
  if (CertainedEdges[L].FromId > FromId)
    return -1;
  if (CertainedEdges[R].FromId < FromId)
    return -1;
  if (CertainedEdges[L].FromId != FromId) {
    while (R - L > 1) {
      int M = (L + R) / 2;
      if (CertainedEdges[M].FromId < FromId) L = M;
      else R = M;
    }
    if (CertainedEdges[R].FromId > FromId)
      return -1;
    L = R;
    R = CertainedEdges.size() - 1;
  }
  if (CertainedEdges[L].FromRound > FromRound)
    return -1;
  if (CertainedEdges[R].FromRound < FromRound)
    return -1;
  if (CertainedEdges[L].FromRound != FromRound) {
    while (R - L > 1) {
      int M = (L + R) / 2;
      if (CertainedEdges[M].FromRound < FromRound) L = M;
      else R = M;
    }
    if (CertainedEdges[R].FromRound > FromRound)
      return -1;
    L = R;
    R = CertainedEdges.size() - 1;
  }
  if (CertainedEdges[L].ToId > ToId)
    return -1;
  if (CertainedEdges[R].ToId < ToId)
    return -1;
  if (CertainedEdges[L].ToId != ToId) {
    while (R - L > 1) {
      int M = (L + R) / 2;
      if (CertainedEdges[M].ToId < ToId) L = M;
      else R = M;
    }
    if (CertainedEdges[R].ToId > ToId)
      return -1;
    L = R;
  }
  StrogeInCache = CertainedEdges[L].StrogeInCache;
  return CertainedEdges[L].ToRound;
}

void RefixRetiming(NodeGenerator &ng) {
  sort(CertainedEdges.begin(), CertainedEdges.end());
  for (int i = 0; i < ng.StartTable.size(); ++ i)
    ng.StartTable[i].Retiming = 0;
  queue<Node> Que;
  int PeriodTime = ng.UpBound;
  for (int R = 1; R <= ng.UpRound; ++ R) {
    for (int i = 1; i <= TotalNode; ++ i)
      if (NodeList[i].InDegree == 0)
        Que.push(ng.GetNode(i, R));
  }
  while (!Que.empty()) {
    Node NowNode = Que.front();
    Que.pop();

    vector<Edge> OutEdges = EdgeList[NowNode.Id];
    for (int j = 0; j < OutEdges.size(); ++ j) {
      bool StrogeInCache = false;
      int ToRound = CheckEdge(NowNode.Id, NowNode.Round, OutEdges[j].To, StrogeInCache);
      Node ToNode = Node();
      if (ToRound == -1) {
        ToNode = ng.GetNodeByColor(OutEdges[j].To, NowNode.Color);
      }
      else {
        ToNode = ng.GetNode(OutEdges[j].To, ToRound);
      }
      long long Cost = (StrogeInCache ? OutEdges[j].CacheTimeCost : OutEdges[j].DRAMTimeCost);
      ToNode.Retiming = NowNode.Retiming + Ceil(NowNode.EndTime + Cost - ToNode.StartTime, PeriodTime);
      ng.SetNodeTime(ToNode);
    }
  }
}


long long CalcTotalTime(long long Prelogue, int PeriodTime, int UpRound, int UpBound) {
  int X = Ceil(PeriodTime - UpRound, UpRound);
  long long TotalTime = Prelogue + 1LL * X * UpBound;
  return TotalTime;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);
  vector<Node> KeyNodeSet = GetKeyNodeSet();
  printf("Top 10 KeyNode\n");
  for (int i = 0; i < KeyNodeSet.size() && i < 10; ++ i)
    KeyNodeSet[i].Show();
  for (int i = 0; i < NgList.size(); ++ i) {
    printf("\nUpBound:%d\tUpRound:%d\n", NgList[i].UpBound, NgList[i].UpRound);
    assert(NgList[i].UpBound <= MAXM);
    memset(Checked, false, sizeof(Checked));
    Visit.clear();
    for (int j = 1; j <= NgList[i].NeedPE; ++ j) {
      Build(j, 1, NgList[i].UpBound, 1);
    }

    // NgList[i].ShowEach(false);
    SpreadKeyNodeSet(KeyNodeSet, NgList[i]);
    printf("Spread KeyNode Set\n");
    // NgList[i].Show();
    AscertainNodes(NgList[i]);
    printf("Ascertain Nodes\n");
    // NgList[i].Show();
    RefixRetiming(NgList[i]);
    NgList[i].CalcPrelogue();

  }

  long long TotalCost = 0;
  for (int i = 1; i <= TotalNode; ++ i)
    TotalCost = TotalCost + NodeList[i].Cost;
  int Launches = Ceil(TotalPE, NgList[0].NeedPE);

  FinalResult FR = FinalResult();

  if (NgList.size() == 1) {
    int Each = Ceil(PeriodTimes, Launches);
    FR.TotalTime = CalcTotalTime(NgList[0].Prelogue, Each, NgList[0].UpRound, NgList[0].UpBound);
    FR.Prelogue = NgList[0].Prelogue;
    FR.Retiming = NgList[0].Retiming;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  else {
    for (int EachX = 0; EachX <= PeriodTimes; ++ EachX) {
      int EachY = PeriodTimes - EachX;
      long long TotalTimeX = CalcTotalTime(NgList[0].Prelogue, EachX, NgList[0].UpRound, NgList[0].UpBound);
      long long TotalTimeY = CalcTotalTime(NgList[1].Prelogue, EachY, NgList[1].UpRound, NgList[1].UpBound);
      long long TotalTime = max(TotalTimeX, TotalTimeY);
      if (FR.TotalTime == -1 || TotalTime < FR.TotalTime)
        FR.TotalTime = TotalTime;
      if (TotalTimeX > TotalTimeY)
        break;
    }
    FR.Prelogue = NgList[0].Prelogue;
    FR.Retiming = NgList[0].Retiming;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  FR.RunOnCache = RunOnCache;
  FR.RunOnDRAM = RunOnDRAM;
  return FR;
}