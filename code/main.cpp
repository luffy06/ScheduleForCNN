

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

#define theory 4
#define experiment 1
const double rate = 0.2;

#if theory == 1
  #include "theory1.h"
#elif theory == 2
  #include "theory2.h"
#elif theory == 3
  #include "theory3.h"
#else
  #include "theoryDAC.h"
#endif

int total_pe, period_times;

void input() {
  srand((unsigned int)time(NULL));
  int line, trash, datatest;
#if experiment == 1
  scanf("%d%d", &total_node, &line);
  FILE* fp = fopen("config.in", "r");
  char op[20];
  fscanf(fp, "%s%d", op, &datatest);
  fscanf(fp, "%s%d", op, &total_pe);
  fscanf(fp, "%s%d", op, &period_times);
  #if theory == 3
    fscanf(fp, "%s%d", op, &upround);
  #elif theory == 4
    fscanf(fp, "%s%d", op, &upround);
  #endif
  fclose(fp);
#elif theory == 3
  scanf("%d%d%d%d%d", &total_node, &line, &total_pe, &period_times, &upround);
#else
  scanf("%d%d%d%d%d", &total_node, &line, &total_pe, &period_times, &trash);
#endif

#if LZD == 1
  period_times = 8;
#endif

  for (int i = 1; i <= total_node; i++) {
    double cost;
    char name[200];
    char op[200];
    scanf("%d%s%s%lf", &nodelist[i].id, name, op, &cost);
    nodelist[i].id = nodelist[i].id + 1;
    nodelist[i].cost = cost;
  }

  for (int i = 0; i < line; i++) {
    int from, to;
    double cost, memory;
    scanf("%d%d%lf", &from, &to, &memory);
    // memory = rand() % 100 + 100;
    from = from + 1;
    to = to + 1;
    if (datatest == 0)
      cost = nodelist[from].cost * rate;
    else
      cost = memory;
    edgelist[from].push_back(Edge(from, to, cost, memory));
  }
}

bool checkLoop() {
  return true;
}

int main() {
  input();
  if (checkLoop()) {
    solve(total_pe, period_times);
  }
  else {
    printf("There are some loops in this graph\n");
  }
  return 0;
}