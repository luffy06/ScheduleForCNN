/*
  Exploiting_Parallelism_for_Convolutional_Connections_in_Processing-In-Memory_Architecture.pdf  
*/
#include <cstdio>
#include <cassert>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

#define MAXN 6600             // the number of node
#define MAXR 505
#define LIMITEDRATIO 0.8

typedef pair<int, int> P;

void testInput() {
  int t;
  scanf("%d", &t);
}


struct Node {
  int Id;
  double Cost;

  int PEId;       // run on which pe
  int Round;
  double StartTime;
  double EndTime;

  int NumbInq;    // the number of used node

  Node() {
    Round = PEId = -1;
    StartTime = EndTime = -1;
    NumbInq = 0;
  }

  Node(int a, int b) {
    Id = a;
    Cost = b;
    Round = PEId = -1;
    StartTime = EndTime = -1;
    NumbInq = 0;
  }

  void SetTime(double st, double ed) {
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

    NumbInq = a.NumbInq;
  }

  void show() {
    printf("ID:%d\tPE:%d\tRound:%d\tNumb:%d\tST:%.3f\tED:%.3f\tCost:%.3f\n", Id, PEId, Round, NumbInq, StartTime, EndTime, Cost);
  }

};

int MapTopology[MAXN];

struct NodeComparation {
  bool operator() (const Node &a, const Node &b) const {
    if (a.EndTime != b.EndTime)
      return a.EndTime > b.EndTime;
    return a.PEId > b.PEId;    
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
  else if (MapTopology[a.Id] != MapTopology[b.Id])
    return MapTopology[a.Id] < MapTopology[b.Id];
  else if (a.Round != b.Round)
    return a.Round < b.Round;
  else if (a.Cost != b.Cost)
    return a.Cost < b.Cost;
  return a.Id < b.Id;
}

struct NodeGenerator {
  int TotalNode;
  int MaxPE;
  double UpBound;
  int UpRound;
  vector<Node> StartTable;

  NodeGenerator() {
    TotalNode = 0;
    MaxPE = 0;
    UpBound = 0;
    StartTable.clear();
  }

  NodeGenerator(int a, int b, int MaxRound, Node NodeList[MAXN]) {
    TotalNode = a;
    MaxPE = b;
    UpBound = 0;
    StartTable.clear();
    CalcBound(MaxRound, NodeList);
  }

  double Init(Node NodeList[MAXN]) {
    queue<Node> q;
    for (int i = 0; i < MaxPE; i++) {
      Node n = Node(0, 0);
      n.PEId = i;
      n.SetTime(0, 0);
      q.push(n);
    }
    for (int i = 1; i <= TotalNode; i++) {
      for (int j = 1; j <= UpRound; j++) {
        Node Emp = q.front();
        q.pop();

        Node n = Node();
        n.Copy(NodeList[i]);
        n.Round = j;
        n.PEId = Emp.PEId;
        n.SetTime(Emp.EndTime, Emp.EndTime + n.Cost);
        q.push(n);
        UpBound = max(UpBound, Emp.EndTime + n.Cost);
        StartTable.push_back(n);
      }
    }
    // for (int i = 0; i < starttable.size(); i++)
    //   starttable[i].show();
    sort(StartTable.begin(), StartTable.end(), CmpById);
    // calculate the use ratio of cpu
    assert(UpBound != 0);
    double Down = UpBound * MaxPE;
    double Up = 0;
    for (int i = 1; i <= TotalNode; i++) {
      Up = Up + NodeList[i].Cost;
    }
    Up = Up * UpRound;
    assert(Down != 0);
    double Ratio = Up / Down;
    return Ratio;
  }

  /*
  * calculate group
  */
  void CalcBound(int MaxRound, Node NodeList[MAXN]) {
    int TargetRound = 1;
    double MaxRatio = 0;
    for (UpRound = 1; UpRound <= MaxRatio; ++ UpRound) {
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


struct Edge {
  int From;
  int To;
  double Cost;
  double Memory;

  Edge() { }

  Edge(int a, int b, double c, double d) {
    From = a;
    To = b;
    Cost = c;
    Memory = d;
  }
};

vector<Edge> EdgeList[MAXN];
Node NodeList[MAXN];
NodeGenerator ng;
int Topology[MAXN];
int Degree[MAXN];
int TotalNode;

/*
* get topology order
* O(V+E)
*/
void GetTopology() {
  int Count = 0, Iter = 0, Order = 0;
  queue<Node> q;
  for (int i = 1; i <= TotalNode; i++) {
    if (Degree[i] == 0) {
      q.push(NodeList[i]);
    }
  }
  Count = q.size();

  while (!q.empty()) {
    Node f = q.front();
    q.pop();
    Topology[Iter] = f.Id;
    MapTopology[f.Id] = Order;

    Iter = Iter + 1;
    Count = Count - 1;

    for (int i = 0; i < EdgeList[f.Id].size(); i++) {
      Edge e = EdgeList[f.Id][i];
      Degree[e.To] = Degree[e.To] - 1;
      if (Degree[e.To] == 0) {
        q.push(NodeList[e.To]);
      }
    }

    if (Count == 0) {
      Count = q.size();
      Order = Order + 1;
    }
  }
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

  GetTopology();
  ng = NodeGenerator(TotalNode, TotalPE, UpRound, NodeList);
  // printf("Max PE:%d UpBound:%.3f UpRound:%d\n", ng.MaxPE, ng.UpBound, ng.UpRound);
}

void test(int TotalPE, int UpRound) {
  Init(TotalPE, UpRound);
  ng.test(NodeList);
}

void Solve(int TotalPE, int PeriodTimes, int UpRound) {
  // test(TotalPE, UpRound);
  Init(TotalPE, UpRound);
  double TotalTime = 0;

  int LastId = -1;
  for (int i = 1; i <= TotalNode; i++) {
    NodeList[i].Round = 1;
    NodeList[i].Copy(ng.GenerateNextNode(NodeList[i], NodeList));
  }
  for (int j = 0; j < TotalNode; j++) {
    int NodeId = Topology[j];
    assert(NodeId == NodeList[NodeId].Id);
    if (NodeList[NodeId].EndTime > TotalTime) {
      LastId = NodeId;
      TotalTime = NodeList[NodeId].EndTime;
    }
    for (int i = 0; i < EdgeList[NodeId].size(); i++) {
      Edge e = EdgeList[NodeId][i];
      double StartTime = NodeList[NodeId].EndTime + e.Cost;
      for (; NodeList[e.To].StartTime < StartTime; NodeList[e.To].Copy(ng.GenerateNextNode(NodeList[e.To], NodeList)));
    }
  }

  assert(LastId > 0 && LastId <= TotalNode);
  int FirstId = Topology[0];
  int Prelogue = (int)((NodeList[LastId].StartTime - NodeList[FirstId].StartTime) / ng.UpBound);
  for (int r = 2; r <= PeriodTimes; r++) {
    NodeList[LastId].Copy(ng.GenerateNextNode(NodeList[LastId], NodeList));
    TotalTime = max(TotalTime, NodeList[LastId].EndTime);
  }

  double Up = 0;
  for (int i = 1; i <= TotalNode; i++) {
    Up = Up + NodeList[i].Cost;
  }
  Up = Up * PeriodTimes;
  double Down = TotalTime * TotalPE;
  assert(Down != 0);
  double CPURatio = Up / Down;
  printf("Total PE:\t%d\nTotal time:\t%.2f\nCPU Used Ratio:\t%.2f\tPrelogue:%d\n", TotalPE, TotalTime, CPURatio, Prelogue);
}