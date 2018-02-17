struct Node {
  int Id;
  int Cost;

  int InDegree;
  int OutDegree;
  int TopoOrder;

  int PEId;
  int Round;
  int Retiming;
  int StartTime;
  int EndTime;

  Node() {
    Id = -1;
    Cost = 0;
    InDegree = OutDegree = 0;
    TopoOrder = -1;
    PEId = -1;
    Round = -1;
    Retiming = 0;
    StartTime = EndTime = -1;
  }

  void SetTime(int st) {
    StartTime = st;
    EndTime = StartTime + Cost;
    assert(StartTime <= EndTime);
  }

  void Copy(Node t) {
    Id = t.Id;
    Cost = t.Cost;
    InDegree = t.InDegree;
    OutDegree = t.OutDegree;
    TopoOrder = t.TopoOrder;
    PEId = t.PEId;
    Round = t.Round;
    Retiming = t.Retiming;
    StartTime = t.StartTime;
    EndTime = t.EndTime;
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
};

struct Phase {
  int PENumb;
  vector<Node> PELine[MAXPE];
  int PEEndTime[MAXPE];
  int PEStartTime[MAXPE];

  Phase() { }

  Phase(int a, int TotalNode, Node NodeList[MAXN], int OutMaxCost[MAXN]) {
    PENumb = a;
    Init(TotalNode, NodeList, OutMaxCost);
  }

  void Init(int TotalNode, Node NodeList[MAXN], int OutMaxCost[MAXN]) {
    int NowOrder = 0;
    int PEIter = 1;
    int NowStartTime = 0;
    int NextStartTime = 0;
    for (int i = 1; i <= PENumb; ++ i) {
      PEStartTime[i] = INF;
      PEEndTime[i] = 0;
    }
    for (int i = 1; i <= TotalNode; ++ i) {
      Node node = NodeList[i];
      if (node.TopoOrder != NowOrder) {
        PEIter = 1;
        NowStartTime = NextStartTime;
        NowOrder = node.TopoOrder;
      }
      node.Round = 0;
      node.SetTime(NowStartTime);
      PELine[PEIter].push_back(node);
      PEStartTime[PEIter] = min(PEStartTime[PEIter], node.StartTime);
      node.Round = 1;
      node.SetTime(node.EndTime);
      PELine[PEIter].push_back(node);
      NextStartTime = max(NextStartTime, node.EndTime + OutMaxCost[node.Id]);
      PEEndTime[PEIter] = max(PEEndTime[PEIter], node.EndTime);
      PEIter = PEIter + 1;
      if (PEIter == PENumb + 1) {
        PEIter = 1;
        NowStartTime = NextStartTime;
      }
    }
  }

  void Show() {
    for (int i = 1; i <= PENumb; ++ i) {
      printf("PE:%d\t", i);
      int LastStartTime = 0;
      for (int j = 0; j < PELine[i].size(); ++ j) {
        Node node = PELine[i][j];
        if (node.StartTime > LastStartTime) {
          for (int k = LastStartTime; k < node.StartTime; ++ k)
            printf("-");
        }
        for (int k = node.StartTime; k < node.EndTime; ++ k) {
          printf("%c", ('A' + node.Id - 1));
        }
        LastStartTime = node.EndTime;
      }
      printf("\n");
    }
  }
};

struct Iteration {
  
  vector<Phase> PhaseGroup;
  vector<int> Moves;

  Iteration(Phase phase) {
    PhaseGroup.push_back(phase);
    PhaseGroup.push_back(phase);

    for (int i = 1; i <= phase.PENumb; ++ i)
      printf("%d %d\n", phase.PEStartTime[i], phase.PEEndTime[i]);

    Moves.push_back(phase.PEStartTime[phase.PENumb]);
    for (int i = 2; i <= phase.PENumb; ++ i) {
      int j = phase.PENumb + 2 - i;
      int Offset = max(Moves[Moves.size() - 1], phase.PEEndTime[j] - phase.PEStartTime[i]);
      printf("i:%d %d\n", i, Offset);
      Moves.push_back(Offset);
    }
  }

  void Show() {
    int PENumb = PhaseGroup[0].PENumb;
    for (int i = 1; i <= PENumb + 1; ++ i) {
      printf("PE:%d\t", i);
      int LastStartTime = 0;
      if (i <= PENumb) {
        for (int j = 0; j < PhaseGroup[0].PELine[i].size(); ++ j) {
          Node node = PhaseGroup[0].PELine[i][j];
          int StartTime = node.StartTime;
          int EndTime = node.EndTime;
          if (StartTime > LastStartTime) {
            for (int k = LastStartTime; k < StartTime; ++ k)
              printf("-");
          }
          for (int k = StartTime; k < EndTime; ++ k)
            printf("%c", ('A' + node.Id - 1));
          LastStartTime = EndTime;
        }
      }
      if (i > 1) {
        int Offset = Moves[PENumb - i + 1];
        for (int j = 0; j < PhaseGroup[1].PELine[PENumb - i + 2].size(); ++ j) {
          Node node = PhaseGroup[1].PELine[PENumb - i + 2][j];
          int StartTime = node.StartTime + Offset;
          int EndTime = node.EndTime + Offset;
          if (StartTime > LastStartTime) {
            for (int k = LastStartTime; k < StartTime; ++ k)
              printf("-");
          }
          for (int k = StartTime; k < EndTime; ++ k)
            printf("%c", ('A' + node.Id - 1));
          LastStartTime = EndTime;
        }
      }
      printf("\n");
    }
  }
};

vector<Edge> EdgeList[MAXN];
Node NodeList[MAXN];
int OutMaxCost[MAXN];
int Degree[MAXN];
int TotalNode;
int TotalPE, PeriodTimes, UpRound;
int RunOnDRAM;
int RunOnCache;

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

void Init(int TotalPE) {
  memset(Degree, 0, sizeof(Degree));

  for (int i = 1; i <= TotalNode; ++ i) {
    OutMaxCost[i] = 0;
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
      OutMaxCost[i] = max(OutMaxCost[i], e.CacheTimeCost);
    }
  }

  RunOnDRAM = RunOnCache = 0;
  int NeedPE = GetTopology();
  Phase phase = Phase(min(NeedPE, TotalPE), TotalNode, NodeList, OutMaxCost);
  Iteration iteration = Iteration(phase);
  iteration.Show();
}

FinalResult Solve(int TotalPE, int PeriodTimes, int UpRound) {
  Init(TotalPE);
  
  FinalResult FR = FinalResult();
  return FR;
}