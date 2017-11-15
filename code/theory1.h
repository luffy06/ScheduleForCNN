/*
  Graph: 
    A -> B
    A -> C
    B -> D
    C -> D
  
  Schedule:
    1 A B D A B D ..
    2   C     C   ..
    3 A B D A B D ..
    4   C     C   ..
    5

  Step 1: calculate the cost of time and PE for one period 
  Step 2: calculate the total time using all PEs
*/
#include <cstdio>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cassert>

using namespace std;

#define MAXN 50000
#define MAXE 90000
#define MINN 50
#define MOD 100

struct Node {
  int id;
  char name[2 * MINN];
  char op[2 * MINN];
  double cost;

  int peid;       // run on which pe
  double starttime;
  double endtime;

  Node() {
    peid = -1;
    starttime = endtime = -1;
  }

  void setTime(double st, double ed) {
    starttime = st;
    endtime = ed;
  }

  friend bool operator< (Node a, Node b) {
    return a.endtime > b.endtime;
  }
};

struct Edge {
  int from;
  int to;
  double cost;
  double memory;

  Edge() { }

  Edge(int a, int b, double c, double d) {
    from = a;
    to = b;
    cost = c;
    memory = d;
  }
};

vector<Edge> edgelist[MAXE];
Node nodelist[MAXN];
bool vis[MAXN];
int degree[MAXN];
int total_node;

void init() {
  memset(vis, false, sizeof(vis));
  memset(degree, 0, sizeof(degree));

  for (int i = 1; i <= total_node; i++) {
    for (int j = 0; j < edgelist[i].size(); j++) {
      Edge e = edgelist[i][j];
      degree[e.to] = degree[e.to] + 1; 
    }
  }
}

Result solveOnce(int pe_upbound) {
  double total_time = 0;
  int pecount = 0, node_count = 0;
  priority_queue<Node> perunning;
  queue<Node> nodewaiting;
  priority_queue<int, vector<int>, greater<int> > freepe;
  // init free PE queue
  for (int i = 1; i <= pe_upbound; i++)
    freepe.push(i);

  // first calculate those nodes whose indegree is zero
  // if PE is not enough, push those nodes into nodewaiting queue
  for (int i = 1; i <= total_node; i++) {
    if (degree[i] == 0) {
      if (!freepe.empty()) {
        int peid = freepe.top();
        freepe.pop();
        pecount = max(pecount, peid);
        nodelist[i].setTime(0, nodelist[i].cost);
        nodelist[i].peid = peid;
        perunning.push(nodelist[i]);
      }
      else {
        nodelist[i].peid = -1;
        nodewaiting.push(nodelist[i]);
      }
      vis[nodelist[i].id] = true;
    }
  }

  // then calculate those running nodes
  while (!perunning.empty()) {
    // finish the top node
    Node top = perunning.top();
    perunning.pop();
    total_time = max(top.endtime, total_time);
    // free PE
    freepe.push(top.peid);

    // minus the indegree of those dependent nodes 
    for (int i = 0; i < edgelist[top.id].size(); i++) {
      Edge e = edgelist[top.id][i];
      if (!vis[e.to]) {
        degree[e.to] = degree[e.to] - 1;
        if (degree[e.to] == 0) {
          nodelist[e.to].peid = -1;
          nodelist[e.to].starttime = top.endtime + e.cost;
          nodewaiting.push(nodelist[e.to]);
          vis[e.to] = true;
        }
      }
    }

    // calculate waiting nodes
    while (!nodewaiting.empty()) {
      if (freepe.empty())
        break;
      
      Node waited = nodewaiting.front();
      nodewaiting.pop();

      int peid = freepe.top();
      freepe.pop();
      pecount = max(pecount, peid);

      int starttime = max(top.endtime, waited.starttime);
      nodelist[waited.id].peid = peid;
      nodelist[waited.id].setTime(starttime, starttime + waited.cost);
      perunning.push(nodelist[waited.id]);
    }

  }

  double up = 0;
  for (int i = 1; i <= total_node; i++)
    up = up + nodelist[i].cost;
  double down = pecount * total_time;
  assert(down != 0);
  double cpuratio = up / down;
  return Result(total_time, pecount, cpuratio);
}

void solve(int total_pe, int period_times) {
  init();
  Result res = solveOnce(total_pe);
  int meanwhile_period = total_pe / res.pecount;
  int total_turn = (period_times % meanwhile_period == 0 ? period_times / meanwhile_period : period_times / meanwhile_period + 1);
  double total_time = total_turn * res.totaltime;
  printf("Total PE:\t%d\nTotal time:\t%.3f\nCPU Used Ratio:\t%.3f\n", res.pecount, total_time, res.cpuratio);
  // for (int i = 1; i <= total_node; i++) {
  //   printf("ID:%d PE:%d StartTime:%d EndTime:%d Cost:%d\n", nodelist[i].id, nodelist[i].peid, nodelist[i].starttime, nodelist[i].endtime, nodelist[i].cost);
  // }
}