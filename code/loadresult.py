import xlwt, xlsxwriter
import os

resultdir = '../result'
resultexcel = 'output.xlsx'
algorithms = ['lctes', 'resc', 'bala']
pes = [16, 32, 64]
attributes = ['Benchmark', 'Node', 'Edge']
metrics = ['TotalTime', 'Prologue', 'RunOnCacheN', 'CPURatio', 'Cost']
drop_benchmark = []

def parse_graph(line):
  lines = line.split(' ')
  return lines[len(lines) - 1]

def parse_filename(filename):
  names = filename.split('.')
  assert(len(names) == 2)
  return (names[0], names[1])

def parse_trace(line):
  ls = line.split(' ')
  trace_name = ls[1].split('.')[0]
  return trace_name, ls[2]

def parse_content(line):
  res = {}
  for l in line.split(' '):
    maps = l.split(':')
    if len(maps) != 2:
      print(l)
    assert(len(maps) == 2)
    res[maps[0]] = maps[1]
  return res

def parse_filecontent(filename, resultmap, peid, attribute):
  f = open(filename, 'r')
  lines = f.readlines();
  f.close()
  attr = []
  attr_set = set()
  for i in range(0, len(lines), 2):
    benchmark, algo = parse_trace(lines[i].strip())
    data = parse_content(lines[i + 1].strip())
    if benchmark in drop_benchmark:
      continue
    if benchmark not in attr_set:
      attr.append([benchmark, data['TotalNode'], data['TotalEdge']])
      attr_set.add(benchmark)
    for m in metrics:
      resultmap[m][algo][peid].append([benchmark, data[m]])
  if len(attribute) == 0:
    for a in attr:
      attribute.append(a)
  else:
    assert(len(attr) == len(attribute))
    for i, a in enumerate(attr):
      for j in range(3):
        assert(a[j] == attribute[i][j])

def loadresult(attribute, resultmap):
  wb = xlsxwriter.Workbook(os.path.join(resultdir, resultexcel))
  st = wb.add_worksheet()
  r = 0
  for m in metrics:
    st.write(r, 0, m)
    r = r + 1

    # write attributes
    for j, a in enumerate(attributes):
      st.write(r, j, a)

    for j, a in enumerate(algorithms):
      st.write(r - 1, j * len(pes) + len(attributes), a)
      for k, p in enumerate(pes):
        st.write(r, j * len(pes) + len(attributes) + k, p)
    r = r + 1

    # write trace
    for i, a in enumerate(attribute):
      for j, v in enumerate(a):
        st.write(r, j, v)
      for j, algo in enumerate(algorithms):
        for k, p in enumerate(pes):
          st.write(r, j * len(pes) + len(a) + k, resultmap[m][algo][p][i][1])
      r = r + 1
    r = r + 1
  wb.close()

def main():
  resultmap = {}

  for m in metrics:
    resultmap[m] = {}
    for algo in algorithms:
      resultmap[m][algo] = {}
      for pe in pes:
        resultmap[m][algo][pe] = []

  attr = []
  for f in os.listdir(resultdir):
    peid, suffix = parse_filename(f)
    if suffix != 'out':
      continue
    if int(peid) not in pes:
      continue
    print('Loading ' + peid)
    parse_filecontent(os.path.join(resultdir, f), resultmap, int(peid), attr)

  # print(resultmap)
  loadresult(attr, resultmap)

if __name__ == '__main__':
  # test()
  main()