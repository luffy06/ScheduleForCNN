#include <cstdio>
#include <cassert>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>

using namespace std;

struct FinalResult {
  int TotalTime;
  int Prelogue;
  int Retiming;
  int RunOnCache;
  int RunOnDRAM;
  double CPURatio;

  FinalResult() {
    TotalTime = 0;
    Prelogue = 0;
    Retiming = 0;
    RunOnCache = 0;
    RunOnDRAM = 0;
    CPURatio = 0;
  }

  FinalResult(int a, int b, int c, int d, int e, double f) {
    TotalTime = a;
    Prelogue = b;
    Retiming = c;
    RunOnCache = d;
    RunOnDRAM = e;
    CPURatio = f;
  }

  void Show() {
    printf("TotalTime:%d\nPrelogue:%d\nRetiming:%d\nRunOnCache:%d\nRunOnDRAM:%d\nCPURatio:%.3f\n", 
            TotalTime, Prelogue, Retiming, RunOnCache, RunOnDRAM, CPURatio);
  }
};

#define THEORY 1
#define EXPERIMENT 1
#define MAXM 70000
#define MAXN 6600             // the number of node
#define MAXPE 600
#define MAXR 505
#define LIMITEDRATIO 0.9
#define ALPHA 0.8
const int INF = 0x3f3f3f3f;
const double DRAMSPEED = 10;
const double CACHESPEED = 100;
const double CACHESIZE = 4;

typedef pair<int, int> TwoInt;
typedef pair<int, TwoInt> ThreeInt;
typedef pair<TwoInt, TwoInt> FourInt;
typedef pair<double, double> TowDouble;

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

  int MaxCost = -1;
  for (int i = 1; i <= TotalNode; i++) {
    int Cost;
    char Name[200];
    char Op[200];
    scanf("%d%s%s%d", &NodeList[i].Id, Name, Op, &Cost);
    NodeList[i].Id = NodeList[i].Id + 1;
    NodeList[i].Cost = Cost;
    MaxCost = max(MaxCost, Cost);
  }

  for (int i = 0; i < Line; i++) {
    int From, To;
    int Memory;
    scanf("%d%d%d", &From, &To, &Memory);
    From = From + 1;
    To = To + 1;
    EdgeList[From].push_back(Edge(From, To, Memory));
    ReEdgeList[To].push_back(Edge(From, To, Memory));
  }
}

int main() {
  Input();
  FinalResult FR = Solve(TotalPE, PeriodTimes, UpRound);
  FR.Show();
  return 0;
}