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

bool CmpById(Node a, Node b) {
  if (a.Id != b.Id)
    return a.Id < b.Id;
  return a.Round < b.Round;
}

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

  NodeGenerator() {
    TotalNode = 0;
    UpBound = 0;
    Prelogue = -1;
    RunOnCache = RunOnDRAM = 0;
    StartTable.clear();
  }

  NodeGenerator(int a, int b, int MaxRound, Node NodeList[MAXN]) {
    TotalNode = a;
    NeedPE = b;
    UpBound = 0;
    Prelogue = -1;
    RunOnCache = RunOnDRAM = 0;
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

  Node FindInStartTable(int Numb, int Id) {
    int L = 0, R = StartTable.size();
    int StIndex = 0;
    assert(StartTable.size() != 0);
    if (StartTable[0].Id != Id) {
      while (R - L > 1) {
        int M = (L + R) >> 1;
        if (StartTable[M].Id < Id)
          L = M;
        else
          R = M;
      }
      StIndex = R;
    }
    Node Res = Node();
    while (StIndex < StartTable.size()) {
      if (StartTable[StIndex].Round == Numb) {
        Res.Copy(StartTable[StIndex]);
        break;
      }
      StIndex = StIndex + 1;
    }
    assert(Res.PEId != -1);
    return Res;
  }

  Node GenerateNextNode(Node n, Node NodeList[MAXN]) {
    Node Res = Node(n.Id, n.Cost);
    Res.Copy(n);
    Res.NumbInq = Res.NumbInq + 1;
    assert(UpRound != 0);
    int Base = (Res.NumbInq % UpRound == 0 ? Res.NumbInq / UpRound - 1 : Res.NumbInq / UpRound);
    int Numb = (Res.NumbInq - 1) % UpRound + 1;
    // PP p = getStartTime(Res.id, numb, NodeList);
    // Res.starttime = base * upbound + p.first;
    Node f = FindInStartTable(Numb, Res.Id);
    Res.StartTime = Base * UpBound + f.StartTime;
    Res.EndTime = Res.StartTime + Res.Cost;
    Res.PEId = f.PEId;
    return Res;
  }

  void test(Node NodeList[MAXN]) {
    int TestRound = 9;
    for (int j = 1; j <= TotalNode; j++) {
      Node st = Node();
      st.Copy(NodeList[j]);
      st.SetTime(0, 0);
      st.NumbInq = 0;
      printf("TEST Node:%d Cost:%.3f\n", NodeList[j].Id, NodeList[j].Cost);
      for (int i = 0; i < TestRound; i++) {
        st.Copy(GenerateNextNode(st, NodeList));
        printf("Numb:%d\tST:%.3f\tED:%.3f\tPE:%d\n", st.NumbInq, st.StartTime, st.EndTime, st.PEId);
      }
      printf("\n");
    }
  }
};

vector<Edge> EdgeList[MAXN];
vector<Edge> ReEdgeList[MAXN];
Node NodeList[MAXN];
NodeGenerator ng;
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;

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
  memset(Topology, 0, sizeof(Topology));
  memset(Degree, 0, sizeof(Degree));

  for (int i = 1; i <= TotalNode; i++) {
    for (int j = 0; j < EdgeList[i].size(); j++) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
    }
  }

  int NeedPE = GetTopology();
  // ng = NodeGenerator(TotalNode, TotalPE, UpRound, NodeList);
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

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE, UpRound);

}