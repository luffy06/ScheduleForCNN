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
  for (int i = 0; i < Line; i++) {
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

int father[MAXN];
int ct[MAXN];
int maxnode[MAXN];

int find_father(int x) {
  if (father[x] != x)
    return father[x] = find_father(father[x]);
  return father[x];
}


int UnionAndFind() {
  for (int i = 1; i <= TotalNode; ++ i)
    father[i] = i;
  
  for (int i = 1; i <= TotalNode; ++ i) {
    vector<Edge> Edges = EdgeList[i];
    for (int j = 0; j < Edges.size(); ++ j) {
      Edge e = Edges[j];
      int fu = find_father(e.From);
      int fv = find_father(e.To);
      if (fu != fv)
        father[fu] = fv;
    }
  }

  set<int> s;
  memset(ct, 0, sizeof(ct));
  memset(maxnode, 0, sizeof(maxnode));
  for (int i = 1; i <= TotalNode; ++ i) {
    father[i] = find_father(i);
    ct[father[i]] = ct[father[i]] + 1;
    maxnode[father[i]] = max(maxnode[father[i]], (int)NodeList[i].Cost);
    s.insert(father[i]);
  }
  int max_ct = 0;
  int f = 0;
  for (auto it : s) {
    if (ct[it] > max_ct) {
      f = it;
      max_ct = ct[it];
    }
  }
  return f;
}

vector<Node> RestNode;
void Reduce(int f) {
  RestNode.clear();
  for (int i = 1; i <= TotalNode; ++ i) {
    if (father[i] == f)
      RestNode.push_back(NodeList[i]);
  }

  bool Certained = false;
  queue<Node> Buffer;
  for (int i = 0; i < RestNode.size(); ++ i) {
    while (true) {
      Node node = Buffer.front();
      if (node.Certained != Certained)
        break;
      Buffer.pop();

      
    }
  }
}

int main() {
  Input();
  GetTopology();
  int f = UnionAndFind();
  printf("Father:%d\n", f);

  return 0;
}