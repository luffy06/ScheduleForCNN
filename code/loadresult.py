import xlwt, xlsxwriter
import os

resultdir = '../result'
resultexcel = 'output.xlsx'
attributes = ['TotalTime', 'Prelogue', 'Retiming', 'RunOnCache', 'RunOnDRAM', 'MAXRatio', 'CPURatio'];

def parse_graph(line):
  lines = line.split(' ')
  return lines[len(lines) - 1]

def parse_filename(filename):
  names = filename.split('.')
  assert(len(names) == 2)
  return (names[0], names[1])

def parse_content(line, resultmap, peid, algo):
  onemap = {}
  for l in line.split(' '):
    key, value = l.split(':')
    onemap[key] = value

  for key in onemap:
    if key in resultmap:
      resultmap[key][algo][peid].append(onemap[key])
    else:
      print('not find key:' + key)

def parse_filecontent(filename, resultmap, peid):
  f = open(filename, 'r')
  lines = f.readlines();
  f.close()
  theory_start = False
  algo = -1
  for l in lines:
    l = l.strip('\n');
    if l.startswith('Dealing'):
      graph_name = parse_graph(l)
      # print(graph_name)
    elif l.startswith('##'):
      if theory_start == False:
        theory_start = True
        for i in l:
          if i >= '0' and i <= '9':
            algo = int(i)
        assert(algo != -1)
      else:
        theory_start = False
        algo = -1
    elif theory_start == True:
      parse_content(l, resultmap, peid, algo - 1)

def loadresult(resultmap):
  wb = xlsxwriter.Workbook(os.path.join(resultdir, resultexcel))
  st = wb.add_worksheet()
  r = 0
  for attr in resultmap:
    ths = resultmap[attr]
    maxr = r
    c = 0
    st.write(r, c, attr)
    r = r + 1
    for thmap in ths:
      for p in [32, 64, 128, 256]:
        for j in range(len(thmap[p])):
          st.write(r + j, c, thmap[p][j])
          maxr = max(maxr, r + j)
        c = c + 1
    r = maxr + 1
  wb.close()

def main():
  resultmap = {}
  for attr in attributes:
    result = [{}, {}, {}]
    for r in result:
      for p in [32, 64, 128, 256]:
        r[p] = []
    resultmap[attr] = result

  for f in os.listdir(resultdir):
    # if os.path.isfile(f) == False:
    #   continue
    peid, suffix = parse_filename(f)
    if suffix != 'out':
      continue
    print('Loading ' + peid)
    parse_filecontent(os.path.join(resultdir, f), resultmap, int(peid))

  # print(resultmap)
  loadresult(resultmap)

if __name__ == '__main__':
  # test()
  main()