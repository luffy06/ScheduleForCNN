#include <cstdio>
#include <cassert>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

struct Result {
  double totaltime;
  int pecount;
  double cpuratio;

  Result() {}

  Result(double a, int b, double c) {
    totaltime = a;
    pecount = b;
    cpuratio = c;
  }
};

#define THEORY 2
#define EXPERIMENT 1
#define MAXN 6600             // the number of node
#define MAXR 505
#define LIMITEDRATIO 0.8
const double DRAMSPEED = 10;
const double CACHESPEED = 100;
const double CACHESIZE = 1000;

typedef pair<int, int> P;

void TestInput() {
  int t;
  scanf("%d", &t);
}

#if THEORY == 1
  #include "theory.h"
#elif THEORY == 2
  #include "theoryDAC.h"
#elif THEORY == 3
  #include "theoryLCTES.h"
#endif

int TotalPE, PeriodTimes, UpRound;

void ReadConfig() {
  FILE* Fp = fopen("config.in", "r");
  char Op[20];
  fscanf(Fp, "%s%d", Op, &TotalPE);
  fscanf(Fp, "%s%d", Op, &PeriodTimes);
  fscanf(Fp, "%s%d", Op, &UpRound);
  fclose(Fp);  
}

void Input() {
  srand((unsigned int)time(NULL));
  int Line;
#if EXPERIMENT == 1
  scanf("%d%d", &TotalNode, &Line);
  ReadConfig();
#else
  scanf("%d%d%d%d%d", &TotalNode, &Line, &TotalPE, &PeriodTimes, &UpRound);
#endif

  double MaxCost = -1;
  for (int i = 1; i <= TotalNode; i++) {
    double Cost;
    char Name[200];
    char Op[200];
    scanf("%d%s%s%lf", &NodeList[i].Id, Name, Op, &Cost);
    NodeList[i].Id = NodeList[i].Id + 1;
    NodeList[i].Cost = Cost;
    MaxCost = max(MaxCost, Cost);
  }
  printf("Max Edge:%.3f\n", MaxCost);

  for (int i = 0; i < Line; i++) {
    int From, To;
    double Cost, Memory;
    scanf("%d%d%lf", &From, &To, &Memory);
    From = From + 1;
    To = To + 1;
    EdgeList[From].push_back(Edge(From, To, Cost, Memory));
  }
}

int main() {
  Input();
  Solve(TotalPE, PeriodTimes, UpRound);
  return 0;
}