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
#endif

void ReadConfig() {
  FILE* Fp = fopen("config.in", "r");
  char Op[20];
  fscanf(Fp, "%s%d", Op, &TotalPE);
  fscanf(Fp, "%s%d", Op, &PeriodTimes);
  fscanf(Fp, "%s%d", Op, &UpRound);
  fclose(Fp);  
}

void Input() {
  int Line;
  scanf("%d%d", &TotalNode, &Line);
  ReadConfig();
  long long MaxCost = -1;
  for (int i = 1; i <= TotalNode; i++) {
    long long Cost;
    char Name[200];
    char Op[200];
    scanf("%d%s%s%lld", &NodeList[i].Id, Name, Op, &Cost);
    assert(Cost >= 0);
    NodeList[i].Id = NodeList[i].Id + 1;
    Cost = Cost + 1;
    assert(Cost > 0);
    NodeList[i].Cost = Cost;
    MaxCost = max(MaxCost, Cost);
  }

  if (MaxCost >= MAXM) {
    printf("Reduce Cost\n");
    for (int i = 1; i <= TotalNode; i++) {
      NodeList[i].Cost = ceil((NodeList[i].Cost * 1.0 / MaxCost) * MAXM / 2);
    }
  }

  long long MaxEdge = -1;
  int MaxDis = -1;
  int MinDis = INF;
  for (int i = 0; i < Line; i++) {
    int From, To;
    long long Memory;
    scanf("%d%d%lld", &From, &To, &Memory);
    From = From + 1;
    To = To + 1;
    Edge e = Edge(From, To, Memory);
    int Dis = Ceil(Memory, CACHESPEED) / NodeList[From].Cost;
    if (Dis < 0) {
      printf("Dis:%d\n", Dis);
      printf("Cost:%lld\n", NodeList[From].Cost);
      e.Show();
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
  printf("MinDis:%d\tMaxDis:%d\tMaxEdge:%lld\n", MinDis, MaxDis, Ceil(MaxEdge, CACHESPEED));
}

int main() {
  Input();
  FinalResult FR = Solve(TotalPE, PeriodTimes, UpRound);
  FR.Show();
  return 0;
}