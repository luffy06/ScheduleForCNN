#!/bin/bash
datafolder="../caffe_data"
# datafolder="../tensorflow_data"
resultfolder="../result"
suffix=".in"
algos=(ext algo lctes base)
pes=(16 32 64 128)
for pe in ${pes[*]}; do
  echo 'CALC PE-'$pe
  if [[ -f 'config.in' ]]; then
    rm 'config.in'
  fi
  resultname=${resultfolder}/${pe}.out
  if [[ -f ${resultname} ]]; then
    rm ${resultname}
  fi
  echo 'TOTAL_PE '$pe >> config.in
  echo 'PERIOD_TIMES 6000' >> config.in
  echo 'UPROUND 300' >> config.in
  for file in ${datafolder}/*${suffix}; do
    filename=`basename $file`
    echo 'GRAPH '$filename
    for algo in ${algos[*]}; do
      echo 'RUNNING '$algo
      echo 'RUNNING '$filename' '$algo >> ${resultname}
      echo `./run_$algo < $datafolder/$filename` >> ${resultname}
    done
  done
done
echo 'LOADING RESULT'
# echo `python3 loadresult.py`