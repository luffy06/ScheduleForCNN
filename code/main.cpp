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

#define TEST 0
#define THEORY 4

#include "util.h"
#include "memory_manager.h"

#if THEORY == 1
  #include "theory_initial.cpp"
#elif THEORY == 2
  #include "theory_reschedule.cpp"
#elif THEORY == 3
  #include "theory_balanced.cpp"
#elif THEORY == 4
  #include "theory_lctes.cpp"
#elif THEORY == 5
  #include "theory_baseline.h"
#endif

void ReadConfig() {
  FILE* fp = fopen("config.in", "r");
  char Op[20];
  fscanf(fp, "%s%d", Op, &total_pe);
  fscanf(fp, "%s%d", Op, &total_rounds);
  fscanf(fp, "%s%d", Op, &round_limit);
  fclose(fp);  
}

void GenPECommunication() {
  int n = 1;
  int m = 1;
  for (int i = 1; i <= total_pe; ++ i) {
    if (total_pe % i == 0 && i < total_pe / i) {
      n = i;
      m = total_pe / i;
    }
  }

  vector<TwoInt> points;
  for (int i = 0; i < n; ++ i)
    for (int j = 0; j < m; ++ j)
      points.push_back(make_pair(i, j));

  int rand_cost = rand() % 10;
  memset(pe_edges, 0, sizeof(pe_edges));
  for (int i = 0; i < points.size(); ++ i) {
    for (int j = 0; j < points.size(); ++ j) {
      int elucid_dis = abs(points[i].first - points[j].first) + abs(points[i].second - points[j].second);
      pe_edges[i + 1][j + 1] = elucid_dis * rand_cost;
    }
  }
}

void Input() {
  ReadConfig();
  GenPECommunication();
  scanf("%d%d", &total_node, &total_edge);
  long long max_cost = -1;
  total_cost = 0;
  for (int i = 1; i <= total_node; i++) {
    long long cost;
    char name[200];
    char operation[200];
    scanf("%d%s%s%lld%d", &node_list[i].id, node_list[i].name, operation, &cost, &node_list[i].layer);
    assert(cost >= 0);
    node_list[i].id = node_list[i].id + 1;
    #if TEST == 0
      cost = cost + 1;
    #endif
    assert(cost > 0);
    node_list[i].cost = cost;
    total_cost = total_cost + cost;
    max_cost = max(max_cost, cost);
  }

  // if (max_cost >= MAXM) {
  //   // printf("Reduce cost\n");
  //   for (int i = 1; i <= total_node; i++) {
  //     node_list[i].cost = ceil((node_list[i].cost * 1.0 / max_cost) * MAXM / 2);
  //   }
  // }
  long long max_edge = -1;
  long long max_memory = -1;
  long long min_memory = LLINF;
  long long memory_sum = 0;
  int count = 0;
  for (int i = 0; i < total_edge; i++) {
    int from, to;
    long long memory;
    scanf("%d%d%lld", &from, &to, &memory);
    memory = Ceil(memory, 1024L);
    from = from + 1;
    to = to + 1;
    #if TEST == 0
      memory = memory + 1;
    #endif
    memory_sum = memory_sum + memory;
    if (memory >= CACHESIZE)
      count = count + 1;
    Edge e = Edge(from, to, 1, memory);
    int dis = Ceil(memory, CACHESPEED) / node_list[from].cost;
    if (dis < 0) {
      // printf("Dis:%d\n", Dis);
      // printf("cost:%lld\n", node_list[From].cost);
      // e.Show();
    }
    assert(dis >= 0);
    max_edge = max(max_edge, memory);
    max_memory = max(max_memory, memory);
    min_memory = min(min_memory, memory);
    node_list[from].out_degree = node_list[from].out_degree + 1;
    node_list[to].in_degree = node_list[to].in_degree + 1;
    edge_list[from].push_back(e);
    re_edge_list[to].push_back(e);
  }
  printf("MemorySum:%lld\tMaxMemory:%lld\tMinMemory:%lld\tExceedCount:%d\n", memory_sum, max_memory, min_memory, count);
  // printf("min_dis:%d\tmax_dis:%d\tmax_edge:%lld\n", min_dis, max_dis, max_edge);
}

void AnalyseGraph() {
  printf("Node:%d\tEdge:%d\n", total_node, total_edge);
  
  int vis[20000];
  memset(degree, 0, sizeof(degree));
  memset(vis, 0, sizeof(vis));
  for (int i = 1; i <= total_node; ++ i) {
    for (int j = 0; j < edge_list[i].size(); ++ j) {
      Edge e = edge_list[i][j];
      degree[e.to] = degree[e.to] + 1;
      node_list[e.from].max_out_edge = max(node_list[e.from].max_out_edge, e.memory);
    }
  }

  int count = 0, max_layer = 0, max_concurrency = 0;
  long long max_cost = 0;
  queue<Node> q;
  for (int i = 1; i <= total_node; ++ i) {
    if (degree[i] == 0) {
      q.push(node_list[i]);
      max_cost = max(max_cost, node_list[i].cost);
      // printf("%d:%s\n", i, node_list[i].name);
    }
  }
  long long total_time = max_cost;
  max_cost = 0;
  max_concurrency = count = q.size();
  // printf("Topo:%d:%d\n", max_layer, count);
  vis[count] = vis[count] + 1;
  while (!q.empty()) {
    Node f = q.front();
    q.pop();
    node_list[f.id].topo_order = max_layer;
    count = count - 1;
    max_cost = max(max_cost, node_list[f.id].cost);

    for (int i = 0; i < edge_list[f.id].size(); ++ i) {
      Edge e = edge_list[f.id][i];
      degree[e.to] = degree[e.to] - 1;
      if (degree[e.to] == 0) {
        q.push(node_list[e.to]);
        // printf("%d:%s\n", e.to, node_list[e.to].name);
      }
    }

    if (count == 0) {
      count = q.size();
      max_concurrency = max(max_concurrency, count);
      max_layer = max_layer + 1;
      // printf("Topo:%d:%d\n", max_layer, count);
      vis[count] = vis[count] + 1;
      total_time = total_time + max_cost;
      max_cost = 0;
    }
  }
  assert(max_cost == 0);

  int max_count = vis[0], index = 0;
  for (int i = 1; i < max_concurrency; ++ i) {
    if (max_count < vis[i]) {
      max_count = vis[i];
      index = i;
    }
  }
  printf("TotalTime:%lld\n", total_time);
  printf("MaxLayer:%d\t(MaxConcurrency:%d, Count:%d)\t(Concurrency:%d, MaxCount:%d)\n", max_layer, max_concurrency, vis[max_concurrency], index, max_count);
}

int main() {
  Input();
  #if THEORY != 0
    long long t = GetTime();
    FinalResult final_result = Solve(total_pe, total_rounds, round_limit);
    final_result.Show(total_node, total_edge);
    long long cost = GetTime() - t;
    printf("Cost:%lld\n", cost);
  #else
    AnalyseGraph();
  #endif
  return 0;
}