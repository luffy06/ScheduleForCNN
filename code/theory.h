struct Node {
  int Id;
  int Cost;

  int PEId;       // run on which pe
  int Round;
  int Retiming;
  int StartTime;
  int EndTime;

  Node() {
    Round = PEId = -1;
    StartTime = EndTime = -1;
    Retiming = 0;
  }

  Node(int a, int b) {
    Id = a;
    Cost = b;
    Round = PEId = -1;
    StartTime = EndTime = -1;
    Retiming = 0;
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
    Retiming = a.Retiming;
    StartTime = a.StartTime;
    EndTime = a.EndTime;
  }

  void Show() {
    printf("ID:%d\tPE:%d\tRound:%d\tRetiming:%d\tST:%d\tED:%d\tCost:%d\n", Id, PEId, Round, Retiming, StartTime, EndTime, Cost);
  }
};

typedef pair<Node, int> NodeInt;

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
  int Retiming;
  vector<Node> StartTable;
  vector<FourInt> Relations;
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

  vector<int> GetNodeInRounds(int NodeId, int Condition, bool Checked[MAXN][MAXR]) {
    vector<int> Rounds;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      if (StartTable[i].Id == NodeId) {
        if (Condition == 1 && !Checked[NodeId][StartTable[i].Round])
          Rounds.push_back(StartTable[i].Round);
        else if (Condition == 2 && StartTable[i].StartTime == -1)
          Rounds.push_back(StartTable[i].Round);
      }
    }
    sort(StartTable.begin(), StartTable.end(), CmpByPE);
    return Rounds;
  }

  vector<Node> GetSamePEOtherNodes(int PEId, int NodeId, int Round) {
    vector<Node> Nodes;
    for (int i = PELoc[PEId]; i < PELoc[PEId + 1]; ++ i) {
      if (StartTable[i].StartTime == -1)
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

  void SetNodeTime(int NodeId, int Round, int Retiming, int StartTime, int EndTime) {
    bool Found = false;
    TwoInt Int = BinarySearch(NodeId);
    int L = Int.first;
    int R = Int.second;
    for (int i = L; i < R; ++ i) {
      if (StartTable[i].Round == Round) {
        StartTable[i].SetTime(StartTime, EndTime);
        StartTable[i].Retiming = Retiming;
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

  bool HasUnCertainedNodes() {
    for (int i = 0; i < StartTable.size(); ++ i)
      if (StartTable[i].StartTime == -1)
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
    Prelogue = (MaxRetiming - MinRetiming + 1) * UpBound;
  }

  void PutRelation(int IdA, int RoundA, int IdB, int RoundB) {
    Relations.push_back(make_pair(make_pair(IdA, RoundA), make_pair(IdB, RoundB)));
  }

  void SortRelations() {
    sort(Relations.begin(), Relations.end(), CmpFourInt);
  }

  bool ExistRelation(int IdA, int RoundA, int IdB) {
    int L = 0;
    int R = Relations.size() - 1;
    assert(Relations[L].first.first <= IdA);
    if (Relations[L].first.first < IdA) {
      while (R - L > 1) {
        int M = (L + R) >> 1;
        if (Relations[M].first.first < IdA)
          L = M;
        else
          R = M;
      }
      assert(Relations[R].first.first == IdA);
      L = R;
      R = Relations.size() - 1;
    }
    assert(Relations[L].first.second <= RoundA);
    if (Relations[L].first.second < RoundA) {
      while (R - L > 1) {
        int M = (L + R) >> 1;
        if (Relations[M].first.second < RoundA)
          L = M;
        else
          R = M;
      }
      assert(Relations[R].first.second == RoundA);
      L = R;
      R = Relations.size() - 1;
    }
    if (Relations[L].second.first > IdB)
      return false;
    if (Relations[L].first.second == IdB)
      return true;
    while (R - L > 1) {
      int M = (L + R) >> 1;
      if (Relations[M].second.first < IdB)
        L = M;
      else
        R = M;
    }
    if (Relations[R].second.first == IdB)
      return true;
    return false;
  }

  void Show() {
    sort(StartTable.begin(), StartTable.end(), CmpByPE);
    int LastPEId = -1;
    int LastEndTime = 0;
    printf("\nArrangement");
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
      if (StartTable[i].StartTime == -1)
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
int DP[MAXN];
TwoInt Trace[MAXN];
vector<PEInterval> PEIntervals[MAXPE];
bool Checked[MAXN][MAXR];
int Cache[MAXPE][MAXM << 2];
int Lazy[MAXPE][MAXM << 2];
vector<Node> Visit;
int RunOnCache;
int RunOnDRAM;

int Ceil(int a, int b) {
  if (a % b == 0)
    return a / b;
  return a / b + 1;
}

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
  vector<int> ArrangedGoods;
  int Sum = 0;
  for (int i = 0; i < Goods.size(); ++ i)
    Sum = Sum + Goods[i];
  printf("SS:%d\n", Sum);
  if (Sum <= BinSize) {
    for (int i = 0; i < Goods.size(); ++ i)
      ArrangedGoods.push_back(i);
    return ArrangedGoods;
  }
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
  int k = BinSize;
  while (Trace[k].first != k) {
    ArrangedGoods.push_back(Trace[k].second);
    k = Trace[k].first;
  }

  assert(ArrangedGoods.size() <= Goods.size());
  sort(ArrangedGoods.begin(), ArrangedGoods.end());
  return ArrangedGoods;  
}

bool ArrangeConnectedNode(int FromNodeId, int KeyNodeTime, int KeyNodePEId, int KeyNodeRetiming, int Direction, int CacheCost, int DRAMCost, NodeGenerator &ng, Node &FromNode) {
  assert(Direction == 1 || Direction == -1);
  vector<int> Rounds = ng.GetNodeInRounds(FromNodeId, 1, Checked);
  if (Rounds.size() == 0)
    return false;

  bool PlaceInCache = true;

  int TargetStartTime = -1;
  int TargetPEId = -1;
  int TargetRound = -1;
  int TargetInt = -1;
  int TargetRetiming = -1;
  int PeriodTime = ng.UpBound;
  // 选取最近的PE
  for (int j = 0; j < Rounds.size(); ++ j) {
    Node FromNode = ng.GetNode(FromNodeId, Rounds[j]);
    int PEId = FromNode.PEId;
    int Retiming = 0;

    int StartTime = KeyNodeTime - Direction * (CacheCost + NodeList[FromNodeId].Cost);
    int EndTime = StartTime + Direction * NodeList[FromNodeId].Cost;
    if (StartTime > EndTime)
      swap(StartTime, EndTime);
    
    if (StartTime < 0) {
      for (int k = StartTime; k < 0; k = k + PeriodTime)
        Retiming = Retiming - 1;
    }
    else if (StartTime >= PeriodTime) {
      for (int k = StartTime; k >= PeriodTime; k = k - PeriodTime)
        Retiming = Retiming + 1;
    }
    printf("%d %d %d\n", Retiming, StartTime, EndTime);
    int Int = -1;
    for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
      if (PEIntervals[PEId][k].StartTime + Retiming * PeriodTime <= StartTime 
        && PEIntervals[PEId][k].EndTime + Retiming * PeriodTime >= EndTime) {
        Int = k;
        break;
      }
    }
    
    if (Int == -1) {
      for (int i = 0; i < 2; ++ i) {
        if (Direction == 1) {
          for (int k = PEIntervals[PEId].size() - 1; k >= 0 ; -- k) {
            if (PEIntervals[PEId][k].EndTime - PEIntervals[PEId][k].StartTime >= NodeList[FromNodeId].Cost
              && PEIntervals[PEId][k].EndTime + (Retiming - i) * PeriodTime <= StartTime) {
                Retiming = Retiming - i;
                Int = k;
                break;
            }
          }
        }
        else {
          for (int k = 0; k < PEIntervals[PEId].size(); ++ k) {
            if (PEIntervals[PEId][k].EndTime - PEIntervals[PEId][k].StartTime >= NodeList[FromNodeId].Cost
             && PEIntervals[PEId][k].StartTime + (Retiming + i) * PeriodTime >= EndTime) {
              Retiming = Retiming + i;
              Int = k;
              break;
            }
          }
        }
      }

      assert(Int != -1);
      if (Direction == 1) {
        EndTime = PEIntervals[PEId][Int].EndTime + Retiming * PeriodTime;
        StartTime = EndTime - NodeList[FromNodeId].Cost;
      }
      else {
        StartTime = PEIntervals[PEId][Int].StartTime + Retiming * PeriodTime;
        EndTime = StartTime + NodeList[FromNodeId].Cost;
      }
    }
    
    printf("%d %d %d\n", Retiming, StartTime, EndTime);
    ng.Show();    
    int BinSize = 0;
    if (Direction == 1)
      BinSize = StartTime - PEIntervals[PEId][Int].StartTime - Retiming * PeriodTime;
    else
      BinSize = PEIntervals[PEId][Int].EndTime + Retiming * PeriodTime - StartTime - NodeList[FromNodeId].Cost;
    vector<Node> SamePENodes = ng.GetSamePEOtherNodes(PEId, FromNodeId, Rounds[j]);
  
    vector<int> NodeSizes;
    for (int k = 0; k < SamePENodes.size(); ++ k)
      NodeSizes.push_back(SamePENodes[k].Cost);
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
  
    int Sum = 0;
    for (int k = 0; k < ArrangedSet.size(); ++ k)
      Sum = Sum + NodeSizes[ArrangedSet[k]];

    printf("%d\n", Sum);
    assert(Sum <= BinSize);
    if (Direction == 1) {
      StartTime = PEIntervals[PEId][Int].StartTime + Retiming * PeriodTime + Sum;
      EndTime = StartTime + NodeList[FromNodeId].Cost;
    }
    else {
      EndTime = PEIntervals[PEId][Int].EndTime + Retiming * PeriodTime - Sum;
      StartTime = EndTime - NodeList[FromNodeId].Cost;
    }
    printf("%d %d %d\n", Retiming, StartTime, EndTime);

    if (Direction == 1 && (TargetStartTime == -1 || StartTime > TargetStartTime)) {
      TargetStartTime = StartTime;
      TargetPEId = PEId;
      TargetRound = Rounds[j];
      TargetInt = Int;
      TargetRetiming = Retiming;
    }
    else if (Direction == -1 && (TargetStartTime == -1 || StartTime < TargetStartTime)) {
      TargetStartTime = StartTime;
      TargetPEId = PEId;
      TargetRound = Rounds[j];
      TargetInt = Int;
      TargetRetiming = Retiming;
    }
  }

  assert(TargetRound != -1);
  if (Direction == 1)
    PlaceInCache = (KeyNodeTime - (TargetStartTime - TargetRetiming * PeriodTime) - NodeList[FromNodeId].Cost) < DRAMCost;
  else
    PlaceInCache = ((TargetStartTime - TargetRetiming * PeriodTime) - KeyNodeTime) < DRAMCost;
  FromNode = ng.GetNode(FromNodeId, TargetRound);
  if (KeyNodePEId != -1 && TargetPEId != KeyNodePEId)
    return false;
  if (PEIntervals[TargetPEId][TargetInt].StartTime < TargetStartTime - TargetRetiming * PeriodTime)
    PEIntervals[TargetPEId].push_back(PEInterval(TargetPEId, PEIntervals[TargetPEId][TargetInt].StartTime, TargetStartTime - TargetRetiming * PeriodTime));
  if (TargetStartTime - TargetRetiming * PeriodTime + NodeList[FromNodeId].Cost < PEIntervals[TargetPEId][TargetInt].EndTime)
    PEIntervals[TargetPEId].push_back(PEInterval(TargetPEId, TargetStartTime - TargetRetiming * PeriodTime + NodeList[FromNodeId].Cost, PEIntervals[TargetPEId][TargetInt].EndTime));
  PEIntervals[TargetPEId].erase(PEIntervals[TargetPEId].begin() + TargetInt);
  sort(PEIntervals[TargetPEId].begin(), PEIntervals[TargetPEId].end());

  Checked[FromNodeId][TargetRound] = true;
  ng.SetNodeTime(FromNodeId, TargetRound, TargetRetiming + KeyNodeRetiming, TargetStartTime - TargetRetiming * PeriodTime, TargetStartTime  - TargetRetiming * PeriodTime + NodeList[FromNodeId].Cost);
  FromNode = ng.GetNode(FromNodeId, TargetRound);
  return PlaceInCache;
}

queue<Node> ArrangeKeyNode(Node KeyNode, NodeGenerator &ng) {
  printf("Arrange KeyNode:%d\tRound:%d\tPEId:%d\n", KeyNode.Id, KeyNode.Round, KeyNode.PEId);
  assert(KeyNode.PEId > 0 && KeyNode.PEId <= ng.NeedPE);
  Visit.push_back(KeyNode);
  queue<Node> CertainedNodes;
  // arrange keynode position
  bool Placed = (KeyNode.StartTime != -1);
  int KeyNodeStartTime = KeyNode.StartTime;
  int KeyNodeEndTime = KeyNode.EndTime;
  int KeyNodePEId = KeyNode.PEId;
  if (KeyNodeStartTime == -1) {
    for (int i = 0; i < PEIntervals[KeyNode.PEId].size(); ) {
      if (PEIntervals[KeyNode.PEId][i].EndTime - PEIntervals[KeyNode.PEId][i].StartTime >= KeyNode.Cost) {
        KeyNodeStartTime = PEIntervals[KeyNode.PEId][i].StartTime;
        KeyNodeEndTime = KeyNodeStartTime + KeyNode.Cost;
        Checked[KeyNode.Id][KeyNode.Round] = true;
        ng.SetNodeTime(KeyNode.Id, KeyNode.Round, 0, KeyNodeStartTime, KeyNodeEndTime);
        if (PEIntervals[KeyNode.PEId][i].EndTime - PEIntervals[KeyNode.PEId][i].StartTime == KeyNode.Cost) {
          PEIntervals[KeyNode.PEId].erase(PEIntervals[KeyNode.PEId].begin() + i);
        }
        else {
          PEIntervals[KeyNode.PEId][i].SetTime(KeyNodeEndTime, PEIntervals[KeyNode.PEId][i].EndTime);
          ++ i;
        }
        Placed = true;
        break;
      }
      else {
        ++ i;
      }
    }
  }
  assert(Placed == true); 

  vector<Edge> Edges;
  vector<NodeInt> ReadyForCache;
  int PeriodTime = ng.UpBound;
  int LCost = 0;
  int RCost = 0;
  int MaxRetiming = 0;
  // deal with in edge
  Edges = ReEdgeList[KeyNode.Id];
  for (int i = 0; i < Edges.size(); ++ i) {
    int FromNodeId = Edges[i].From;
    printf("Arrange In-Edge:%d %d\n", FromNodeId, KeyNode.Id);
    Node FromNode = Node();
    if (ArrangeConnectedNode(FromNodeId, KeyNodeStartTime, -1, KeyNode.Retiming, 1, Edges[i].CacheTimeCost, Edges[i].DRAMTimeCost, ng, FromNode)) {
      ReadyForCache.push_back(make_pair(FromNode, Edges[i].Memory));
      MaxRetiming = max(MaxRetiming, FromNode.Retiming);

      int Dis = KeyNodeStartTime - FromNode.StartTime - FromNode.Retiming * PeriodTime - NodeList[FromNodeId].Cost;
      int NLCost = min(KeyNodeStartTime, Dis);
      int NRCost = (Dis <= KeyNodeStartTime ? 0 : Dis - KeyNodeStartTime);
      while (NRCost >= PeriodTime)
        NRCost = NRCost - PeriodTime;
      LCost = max(LCost, NLCost);
      RCost = max(RCost, NRCost);
    }
  }

  int H = 0;
  int BinSize = CACHESIZE;
  if (ReadyForCache.size() > 0) {
  
    if (MaxRetiming == 0) {
      if (LCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, KeyNodeStartTime - LCost + 1, KeyNodeStartTime));
      if (RCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, PeriodTime - RCost + 1, PeriodTime));
    }
    else {
      H = Query(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime);
    }
    
    BinSize = CACHESIZE - H;

    vector<int> NodeSizes;
    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      NodeSizes.push_back(NodeList[ReadyForCache[i].first.Id].Cost);
    }
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
    RunOnCache = RunOnCache + ArrangedSet.size();
    RunOnDRAM = RunOnDRAM + ReadyForCache.size() - ArrangedSet.size();

    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      Node ArNode = ReadyForCache[ArrangedSet[i]].first;
      ng.PutRelation(ArNode.Id, ArNode.Round, KeyNode.Id, KeyNode.Round);
      CertainedNodes.push(ArNode);

      int Memory = ReadyForCache[ArrangedSet[i]].second;
      int Dis = KeyNodeStartTime - ArNode.StartTime - ArNode.Retiming * PeriodTime - ArNode.Cost;
      int NLCost = min(KeyNodeStartTime, Dis);
      int NRCost = (Dis <= KeyNodeStartTime ? 0 : Dis - KeyNodeStartTime);
      while (NRCost >= PeriodTime)
        NRCost = NRCost - PeriodTime;

      for (int j = 0; j < abs(ArNode.Retiming); ++ j)
        Update(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime, Memory);
      if (NLCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, KeyNodeStartTime - NLCost + 1, KeyNodeStartTime, Memory);
      if (NRCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, PeriodTime - NRCost + 1, PeriodTime, Memory);
      Checked[ArNode.Id][ArNode.Round] = true;
      ReadyForCache.erase(ReadyForCache.begin() + ArrangedSet[i]);
    }
    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      Node ArNode = ReadyForCache[i].first;
      Checked[ArNode.Id][ArNode.Round] = true;
      ng.SetNodeTime(ArNode.Id, ArNode.Round, 0, -1, -1);
    }
  }

  // deal with out edge whose pe's id is same
  Edges.clear();
  ReadyForCache.clear();
  LCost = 0;
  RCost = 0;
  MaxRetiming = 0;
  Edges = EdgeList[KeyNode.Id];
  for (int i = 0; i < Edges.size(); ++ i) {
    int ToNodeId = Edges[i].To;
    printf("Arrange Out-Edge:%d %d\n", ToNodeId, KeyNode.Id);
    Node ToNode = Node();
    if (ArrangeConnectedNode(ToNodeId, KeyNodeEndTime, KeyNodePEId, KeyNode.Retiming, -1, Edges[i].CacheTimeCost, Edges[i].DRAMTimeCost, ng, ToNode)) {
      ReadyForCache.push_back(make_pair(ToNode, Edges[i].Memory));
      MaxRetiming = max(MaxRetiming, ToNode.Retiming);

      int Dis = ToNode.StartTime - KeyNodeEndTime;
      int NRCost = min(PeriodTime - KeyNodeEndTime, Dis);
      int NLCost = (Dis <= PeriodTime - KeyNodeEndTime ? 0 : Dis - PeriodTime + KeyNodeEndTime);
      while (NLCost >= PeriodTime)
        NLCost = NLCost - PeriodTime;
      LCost = max(LCost, NLCost);
      RCost = max(RCost, NRCost);
    }
  }

  H = 0;
  if (ReadyForCache.size() > 0) {
    if (MaxRetiming == 0) {
      if (LCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, 1, LCost));
      if (RCost > 0)
        H = max(H, Query(KeyNode.PEId, 1, PeriodTime, 1, KeyNodeEndTime + 1, KeyNodeEndTime + RCost));
    }
    else {
      H = Query(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime);
    }
    
    BinSize = CACHESIZE - H;
    
    vector<int> NodeSizes;
    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      NodeSizes.push_back(NodeList[ReadyForCache[i].first.Id].Cost);
    }
    vector<int> ArrangedSet = ArrangeInFixedSize(NodeSizes, BinSize);
    RunOnCache = RunOnCache + ArrangedSet.size();
    RunOnDRAM = RunOnDRAM + ReadyForCache.size() - ArrangedSet.size();

    for (int i = ArrangedSet.size() - 1; i >= 0; -- i) {
      Node ArNode = ReadyForCache[ArrangedSet[i]].first;
      ng.PutRelation(KeyNode.Id, KeyNode.Round, ArNode.Id, ArNode.Round);
      CertainedNodes.push(ArNode);

      int Memory = ReadyForCache[ArrangedSet[i]].second;
      int Dis = ArNode.StartTime - KeyNodeEndTime;
      int NRCost = min(PeriodTime - KeyNodeEndTime, Dis);
      int NLCost = (Dis <= PeriodTime - KeyNodeEndTime ? 0 : Dis - PeriodTime + KeyNodeEndTime);
      while (NLCost >= PeriodTime)
        NLCost = NLCost - PeriodTime;

      for (int j = 0; j < abs(ArNode.Retiming); ++ j)
        Update(KeyNode.PEId, 1, PeriodTime, 1, 1, PeriodTime, Memory);
      if (NLCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, 1, NLCost, Memory);
      if (NRCost > 0)
        Update(KeyNode.PEId, 1, PeriodTime, 1, KeyNodeEndTime + 1, KeyNodeEndTime + NRCost, Memory);
      Checked[ArNode.Id][ArNode.Round] = true;
      ReadyForCache.erase(ReadyForCache.begin() + ArrangedSet[i]);
    }

    for (int i = 0; i < ReadyForCache.size(); ++ i) {
      Node ArNode = ReadyForCache[i].first;
      Checked[ArNode.Id][ArNode.Round] = true;
      ng.SetNodeTime(ArNode.Id, ArNode.Round, 0, -1, -1);
    }
  }
  ng.Show();
  return CertainedNodes;
}

void SpreadKeyNodeSet(vector<Node> KeyNodeSet, NodeGenerator &ng) {
  queue<Node> CertainedNodes;
  for (int i = 1; i <= ng.NeedPE; ++ i) {
    PEIntervals[i].clear();
    PEIntervals[i].push_back(PEInterval(i, 0, ng.UpBound));
  }
  for (int i = 0; i < KeyNodeSet.size(); ++ i) {
    vector<int> Rounds = ng.GetNodeInRounds(KeyNodeSet[i].Id, 1, Checked);
    for (int j = 0; j < Rounds.size(); ++ j) {
      Node KeyNode = ng.GetNode(KeyNodeSet[i].Id, Rounds[j]);
      CertainedNodes = ArrangeKeyNode(KeyNode, ng);
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
    while (!CertainedNodes.empty()) {
      Node KeyNode = CertainedNodes.front();
      CertainedNodes.pop();
      queue<Node> Nodes = ArrangeKeyNode(KeyNode, ng);
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

void AscertainNodes(NodeGenerator &ng) {
  if (!ng.HasUnCertainedNodes())
    return ;

  int PeriodTime = ng.UpBound;
  for (int i = 0; i < Visit.size(); ++ i) {
    Node KeyNode = Visit[i];

    vector<Edge> Edges;
    Edges = ReEdgeList[KeyNode.Id];
    for (int i = 0; i < Edges.size(); i++) {
      int FromNodeId = Edges[i].From;
      if (ng.ExistRelation(KeyNode.Id, KeyNode.Round, FromNodeId))
        continue;
      vector<int> Rounds = ng.GetNodeInRounds(FromNodeId, 2, Checked);
      assert(Rounds.size() > 0);

      int TargetRound = -1;
      int TargetStartTime = -1;
      for (int i = 0; i < Rounds.size(); ++ i) {
        int StartTime = KeyNode.StartTime - Edges[i].CacheTimeCost - NodeList[FromNodeId].Cost;
        int EndTime = StartTime + NodeList[FromNodeId].Cost;
        int Retiming = 0;
    
        if (StartTime < 0) {
          for (int k = StartTime; k < 0; k = k + PeriodTime)
            Retiming = Retiming - 1;
        }
        // TODO


      }
    }
  }
}

int CalcTotalTime(int Prelogue, int PeriodTime, int UpRound, int UpBound) {
  int X = Ceil(PeriodTime - UpRound, UpRound);
  int TotalTime = Prelogue + X * UpBound;
  return TotalTime;
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);
  vector<Node> KeyNodeSet = GetKeyNodeSet();
  for (int i = 0; i < NgList.size(); ++ i) {
    memset(Checked, false, sizeof(Checked));
    Visit.clear();
    printf("UpBound:%d\n", NgList[i].UpBound);
    NgList[i].Show();
    SpreadKeyNodeSet(KeyNodeSet, NgList[i]);
    NgList[i].SortRelations();
    NgList[i].Show();
    AscertainNodes(NgList[i]);
    NgList[i].Show();
    NgList[i].CalcPrelogue();
  }

  int TotalCost = 0;
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
    int EachX = PeriodTimes / (Launches - 1);
    int EachY = PeriodTimes - EachX * (Launches - 1);
    int TotalTimeX = CalcTotalTime(NgList[0].Prelogue, EachX, NgList[0].UpRound, NgList[0].UpBound);
    int TotalTimeY = CalcTotalTime(NgList[1].Prelogue, EachY, NgList[1].UpRound, NgList[1].UpBound);
    while (TotalTimeX > TotalTimeY) {
      FR.TotalTime = max(TotalTimeX, TotalTimeY);
      EachX = EachX - 1;
      EachY = PeriodTimes - EachX * (Launches - 1);
      TotalTimeX = CalcTotalTime(NgList[0].Prelogue, EachX, NgList[0].UpRound, NgList[0].UpBound);
      TotalTimeY = CalcTotalTime(NgList[1].Prelogue, EachY, NgList[1].UpRound, NgList[1].UpBound);
    }
    FR.Prelogue = NgList[0].Prelogue;
    FR.Retiming = NgList[0].Retiming;
    FR.CPURatio = 1.0 * (PeriodTimes * TotalCost) / (FR.TotalTime * TotalPE);
  }
  FR.RunOnCache = RunOnCache;
  FR.RunOnDRAM = RunOnDRAM;
  return FR;
}