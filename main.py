import os

def read_data(filename):
  f = open(filename, 'r')
  lines = list(map(lambda x : x.strip(), f.readlines()))
  f.close()

  n = int(lines[0].split()[0])
  datas = []
  for i in range(1, n + 1):
    ds = lines[i].split()
    datas.append((ds[1], ds[3]))
  return datas

def subop(op):
  res = ''
  for i in op:
    try:
      int(i)
    except Exception as e:
      res = res + i
    else:
      break
  return res 

def main():
  dirname = 'caffe_data'
  for file in os.listdir(dirname):
    filename = os.path.join(dirname, file)
    datas = read_data(filename)
    t_map = {}
    for d in datas:
      op = subop(d[0])
      t = int(d[1])
      if op in t_map:
        t_map[op] = t_map[op] + t
      else:
        t_map[op] = t
    print('File:' + file)
    for k, v in t_map.items():
      print(k, v)

if __name__ == '__main__':
  main()