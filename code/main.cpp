#include <cstdio>
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <chrono>

using namespace std;

#include "util.h"

#if THEORY == 1
  #include "theory.h"
#elif THEORY == 2
  #include "theoryLCTES.h"
#elif THEORY == 3
  #include "theoryBASE_LS.h"
#elif THEORY == 4
  #include "theory_EXT.h"
#endif

void ReadConfig() {
  FILE* fp = fopen("config.in", "r");
  char Op[20];
  fscanf(fp, "%s%d", Op, &TotalPE);
  fscanf(fp, "%s%d", Op, &PeriodTimes);
  fscanf(fp, "%s%d", Op, &UpRound);
  fclose(fp);  
}

void GenPECommunication() {
  int n = 1;
  int m = 1;
  for (int i = 1; i <= TotalPE; ++ i) {
    if (TotalPE % i == 0 && i < TotalPE / i) {
      n = i;
      m = TotalPE / i;
    }
  }

  vector<TwoInt> points;
  for (int i = 0; i < n; ++ i)
    for (int j = 0; j < m; ++ j)
      points.push_back(make_pair(i, j));

  memset(PEEdge, 0, sizeof(PEEdge));
  int max_speed = CACHESPEED * 100;
  for (int i = 0; i < points.size(); ++ i) {
    for (int j = 0; j < points.size(); ++ j) {
      int dis = abs(points[i].first - points[j].first) + abs(points[i].second - points[j].second);
      if (dis == 0)
        PEEdge[i + 1][j + 1] = 0;
      else
        PEEdge[i + 1][j + 1] = max_speed / dis;
    }
  }
}

void Input() {
  ReadConfig();
  GenPECommunication();
  scanf("%d%d", &TotalNode, &TotalEdge);
  long long MaxCost = -1;
  for (int i = 1; i <= TotalNode; i++) {
    long long Cost;
    char Name[200];
    char Op[200];
    scanf("%d%s%s%lld%d", &NodeList[i].Id, NodeList[i].Name, Op, &Cost, &NodeList[i].Layer);
    assert(Cost >= 0);
    NodeList[i].Id = NodeList[i].Id + 1;
    Cost = Cost + 1;
    assert(Cost > 0);
    NodeList[i].Cost = Cost;
    MaxCost = max(MaxCost, Cost);
  }

  if (MaxCost >= MAXM) {
    // printf("Reduce Cost\n");
    for (int i = 1; i <= TotalNode; i++) {
      NodeList[i].Cost = ceil((NodeList[i].Cost * 1.0 / MaxCost) * MAXM / 2);
    }
  }
  long long MaxEdge = -1;
  int MaxDis = -1;
  int MinDis = INF;
  for (int i = 0; i < TotalEdge; i++) {
    int From, To;
    long long Memory;
    scanf("%d%d%lld", &From, &To, &Memory);
    From = From + 1;
    To = To + 1;
    Memory = Memory + 1;
    Edge e = Edge(From, To, Memory);
    int Dis = Ceil(Memory, CACHESPEED) / NodeList[From].Cost;
    if (Dis < 0) {
      // printf("Dis:%d\n", Dis);
      // printf("Cost:%lld\n", NodeList[From].Cost);
      // e.Show();
    }
    assert(Dis >= 0);
    MaxEdge = max(MaxEdge, Memory);
    MaxDis = max(MaxDis, Dis);
    MinDis = min(MinDis, Dis);
    NodeList[From].OutDegree = NodeList[From].OutDegree + 1;
    NodeList[To].InDegree = NodeList[To].InDegree + 1;
    EdgeList[From].push_back(e);
    ReEdgeList[To].push_back(e);
  }
  // printf("MinDis:%d\tMaxDis:%d\tMaxEdge:%lld\n", MinDis, MaxDis, MaxEdge);
}

void AnalyseGraph() {
  printf("Node:%d\tEdge:%d\n", TotalNode, TotalEdge);
  
  int vis[6000];
  memset(Degree, 0, sizeof(Degree));
  memset(vis, 0, sizeof(vis));
  for (int i = 1; i <= TotalNode; ++ i) {
    for (int j = 0; j < EdgeList[i].size(); ++ j) {
      Edge e = EdgeList[i][j];
      Degree[e.To] = Degree[e.To] + 1;
      NodeList[e.From].MaxOutEdge = max(NodeList[e.From].MaxOutEdge, e.Memory);
    }
  }

  int Count = 0, Order = 0, NeedPE = 0;
  queue<Node> q;
  for (int i = 1; i <= TotalNode; ++ i) {
    if (Degree[i] == 0) {
      q.push(NodeList[i]);
      // printf("%d:%s\n", i, NodeList[i].Name);
    }
  }
  NeedPE = Count = q.size();
  // printf("Topo:%d:%d\n", Order, Count);
  vis[Count] = vis[Count] + 1;
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
        // printf("%d:%s\n", e.To, NodeList[e.To].Name);
      }
    }

    if (Count == 0) {
      Count = q.size();
      NeedPE = max(NeedPE, Count);
      Order = Order + 1;
      // printf("Topo:%d:%d\n", Order, Count);
      vis[Count] = vis[Count] + 1;
    }
  }

  int MaxCount = vis[0], index = 0;
  for (int i = 1; i < Order; ++ i) {
    if (MaxCount < vis[i]) {
      MaxCount = vis[i];
      index = i;
    }
  }
  printf("NeedPE:%d\tMaxLayer:%d\tMaxCount:%d\n", NeedPE, Order, index);

  // sort(NodeList + 1, NodeList + TotalNode + 1, CmpByLayer);
  // Count = 0, Order = 0;
  // NeedPE = 0;
  // for (int i = 1; i <= TotalNode; ++ i) {
  //   if (NodeList[i].Layer != Order) {
  //     // printf("Layer:%d:%d\n", Order, Count);
  //     NeedPE = max(NeedPE, Count);
  //     Count = 0;
  //     Order = NodeList[i].Layer;
  //   }
  //   Count = Count + 1;
  // }
  // NeedPE = max(NeedPE, Count);
  // printf("NeedPE:%d\tMaxLayer:%d\n", NeedPE, Order);
}

long long GetTime() {
  std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()
  );
  return ms.count();
}

int main() {
  Input();
  #if THEORY != 0
    long long t = GetTime();
    FinalResult FR = Solve(TotalPE, PeriodTimes, UpRound);
    FR.Show(TotalNode, TotalEdge);
    long long cost = GetTime() - t;
    printf("Cost:%lld\n", cost);
  #else
    AnalyseGraph();
  #endif
  return 0;
}