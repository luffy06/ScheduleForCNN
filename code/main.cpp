#include <cstdio>
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <cstring>

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
    scanf("%d%s%s%lld%d", &NodeList[i].Id, Name, Op, &Cost, &NodeList[i].Layer);
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
  int NeedPE = GetTopology();
  printf("NeedPE:%d\n", NeedPE);
}

int main() {
  Input();
  AnalyseGraph();
  #if THEORY != 0
    FinalResult FR = Solve(TotalPE, PeriodTimes, UpRound);
    FR.Show(TotalNode, TotalEdge);
  #endif
  return 0;
}