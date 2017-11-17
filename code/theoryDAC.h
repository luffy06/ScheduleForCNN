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
#define LIMITEDRATIO 0.9

typedef pair<int, int> P;

void testInput() {
  int t;
  scanf("%d", &t);
}


struct Node {
  int id;
  double cost;

  int peid;       // run on which pe
  int round;
  double starttime;
  double endtime;

  int numbinq;    // the number of used node

  Node() {
    round = peid = -1;
    starttime = endtime = -1;
    numbinq = 0;
  }

  Node(int a, int b) {
    id = a;
    cost = b;
    round = peid = -1;
    starttime = endtime = -1;
    numbinq = 0;
  }

  void setTime(double st, double ed) {
    starttime = st;
    endtime = ed;
  }

  void copy(Node a) {
    id = a.id;
    cost = a.cost;

    peid = a.peid;
    round = a.round;
    starttime = a.starttime;
    endtime = a.endtime;

    numbinq = a.numbinq;
  }

  void show() {
    printf("ID:%d\tPE:%d\tRound:%d\tNumb:%d\tST:%.3f\tED:%.3f\tCost:%.3f\n", id, peid, round, numbinq, starttime, endtime, cost);
  }

};

int maptopology[MAXN];

struct NodeComparation {
  bool operator() (const Node &a, const Node &b) const {
    if (a.endtime != b.endtime)
      return a.endtime > b.endtime;
    return a.peid > b.peid;    
  }
};

bool cmpByCost(Node a, Node b) {
  if (a.cost != b.cost)
    return a.cost > b.cost;
  return a.id < b.id;
}

bool cmpById(Node a, Node b) {
  if (a.id != b.id)
    return a.id < b.id;
  return a.round < b.round;
}

bool cmpByPE(Node a, Node b) {
  if (a.peid != b.peid)
    return a.peid < b.peid;
  else if (maptopology[a.id] != maptopology[b.id])
    return maptopology[a.id] < maptopology[b.id];
  else if (a.round != b.round)
    return a.round < b.round;
  else if (a.cost != b.cost)
    return a.cost < b.cost;
  return a.id < b.id;
}

struct NodeGenerator {
  int totalnode;
  int maxpe;
  double upbound;
  int upround;
  vector<Node> starttable;

  NodeGenerator() {
    totalnode = 0;
    maxpe = 0;
    upbound = 0;
    starttable.clear();
  }

  NodeGenerator(int a, int b, int maxround, Node nodelist[MAXN]) {
    totalnode = a;
    maxpe = b;
    upbound = 0;
    starttable.clear();
    calcBound(maxround, nodelist);
  }

  double init(Node nodelist[MAXN]) {
    queue<Node> q;
    for (int i = 0; i < maxpe; i++) {
      Node n = Node(0, 0);
      n.peid = i;
      n.setTime(0, 0);
      q.push(n);
    }
    for (int i = 1; i <= totalnode; i++) {
      for (int j = 1; j <= upround; j++) {
        Node emp = q.front();
        q.pop();

        Node n = Node();
        n.copy(nodelist[i]);
        n.round = j;
        n.peid = emp.peid;
        n.setTime(emp.endtime, emp.endtime + n.cost);
        q.push(n);
        upbound = max(upbound, emp.endtime + n.cost);
        starttable.push_back(n);
      }
    }
    // for (int i = 0; i < starttable.size(); i++)
    //   starttable[i].show();
    sort(starttable.begin(), starttable.end(), cmpById);
    // calculate the use ratio of cpu
    assert(upbound != 0);
    double down = upbound * maxpe;
    double up = 0;
    for (int i = 1; i <= totalnode; i++) {
      up = up + nodelist[i].cost;
    }
    up = up * upround;
    assert(down != 0);
    double res = up / down;
    return res;
  }

  /*
  * calculate group
  */
  void calcBound(int maxround, Node nodelist[MAXN]) {
    int targetround = 1;
    double maxratio = 0;
    for (upround = 1; upround <= maxround; upround ++) {
      double useratio = init(nodelist);
      if (useratio >= LIMITEDRATIO) {
        return ;
      }
      else if (useratio > maxratio) {
        targetround = upround;
        maxratio = useratio;
      }
    }
    upround = targetround;
  }

  Node findInStartTable(int numb, int id) {
    int l = 0, r = starttable.size();
    int stindex = 0;
    assert(starttable.size() != 0);
    if (starttable[0].id != id) {
      while (r - l > 1) {
        int m = (l + r) >> 1;
        if (starttable[m].id < id)
          l = m;
        else
          r = m;
      }
      stindex = r;
    }
    Node res = Node();
    while (stindex < starttable.size()) {
      if (starttable[stindex].round == numb) {
        res.copy(starttable[stindex]);
        break;
      }
      stindex = stindex + 1;
    }
    assert(res.peid != -1);
    return res;
  }

  Node generateNextNode(Node n, Node nodelist[MAXN]) {
    Node res = Node(n.id, n.cost);
    res.copy(n);
    res.numbinq = res.numbinq + 1;
    assert(upround != 0);
    int base = (res.numbinq % upround == 0 ? res.numbinq / upround - 1 : res.numbinq / upround);
    int numb = (res.numbinq - 1) % upround + 1;
    // PP p = getStartTime(res.id, numb, nodelist);
    // res.starttime = base * upbound + p.first;
    Node f = findInStartTable(numb, res.id);
    res.starttime = base * upbound + f.starttime;
    res.endtime = res.starttime + res.cost;
    res.peid = f.peid;
    return res;
  }

  void test(Node nodelist[MAXN]) {
    int test_round = 9;
    for (int j = 1; j <= totalnode; j++) {
      Node st = Node();
      st.copy(nodelist[j]);
      st.setTime(0, 0);
      st.numbinq = 0;
      printf("TEST Node:%d Cost:%.3f\n", nodelist[j].id, nodelist[j].cost);
      for (int i = 0; i < test_round; i++) {
        st.copy(generateNextNode(st, nodelist));
        printf("Numb:%d\tST:%.3f\tED:%.3f\tPE:%d\n", st.numbinq, st.starttime, st.endtime, st.peid);
      }
      printf("\n");
    }
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

vector<Edge> edgelist[MAXN];
Node nodelist[MAXN];
NodeGenerator ng;
int topology[MAXN];
int degree[MAXN];
int total_node;
int upround;

/*
* get topology order
* O(V+E)
*/
void getTopology() {
  int count = 0, iter = 0, order = 0;
  queue<Node> q;
  for (int i = 1; i <= total_node; i++) {
    if (degree[i] == 0) {
      q.push(nodelist[i]);
    }
  }
  count = q.size();

  while (!q.empty()) {
    Node f = q.front();
    q.pop();
    topology[iter] = f.id;
    maptopology[f.id] = order;

    iter = iter + 1;
    count = count - 1;

    for (int i = 0; i < edgelist[f.id].size(); i++) {
      Edge e = edgelist[f.id][i];
      degree[e.to] = degree[e.to] - 1;
      if (degree[e.to] == 0) {
        q.push(nodelist[e.to]);
      }
    }

    if (count == 0) {
      count = q.size();
      order = order + 1;
    }
  }
}

void init(int total_pe) {
  memset(topology, 0, sizeof(topology));
  memset(degree, 0, sizeof(degree));

  for (int i = 1; i <= total_node; i++) {
    for (int j = 0; j < edgelist[i].size(); j++) {
      Edge e = edgelist[i][j];
      degree[e.to] = degree[e.to] + 1;
    }
  }

  getTopology();
  ng = NodeGenerator(total_node, total_pe, upround, nodelist);
  // printf("Max PE:%d UpBound:%.3f UpRound:%d\n", ng.maxpe, ng.upbound, ng.upround);
}

void test(int total_pe) {
  init(total_pe);
  printf("TEST NOR\n");
  ng.test(nodelist);
}

void solve(int total_pe, int period_times) {
  // test(total_pe);
  init(total_pe);
  double total_time = 0;

  int lastid = -1;
  for (int i = 1; i <= total_node; i++) {
    nodelist[i].round = 1;
    nodelist[i].copy(ng.generateNextNode(nodelist[i], nodelist));
  }
  for (int j = 0; j < total_node; j++) {
    int node_id = topology[j];
    assert(node_id == nodelist[node_id].id);
    if (nodelist[node_id].endtime > total_time) {
      lastid = node_id;
      total_time = nodelist[node_id].endtime;
    }
    for (int i = 0; i < edgelist[node_id].size(); i++) {
      Edge e = edgelist[node_id][i];
      double starttime = nodelist[node_id].endtime + e.cost;
      for (; nodelist[e.to].starttime < starttime; nodelist[e.to].copy(ng.generateNextNode(nodelist[e.to], nodelist)));
    }
  }
  // for (int i = 1; i <= total_node; i++)
  //   nodelist[i].show();

  assert(lastid > 0 && lastid <= total_node);
  int firstid = topology[0];
  int prelogue = (int)((nodelist[lastid].starttime - nodelist[firstid].starttime) / ng.upbound);
  for (int r = 2; r <= period_times; r++) {
    nodelist[lastid].copy(ng.generateNextNode(nodelist[lastid], nodelist));
    total_time = max(total_time, nodelist[lastid].endtime);
  }

  double up = 0;
  for (int i = 1; i <= total_node; i++) {
    up = up + nodelist[i].cost;
  }
  up = up * period_times;
  double down = total_time * total_pe;
  assert(down != 0);
  double cpuratio = up / down;
  printf("Total PE:\t%d\nTotal time:\t%.2f\nCPU Used Ratio:\t%.2f\tPrelogue:%d\n", total_pe, (total_time) / (1e6), cpuratio, prelogue);
}