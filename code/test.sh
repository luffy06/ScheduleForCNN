#!/bin/bash
datafolder="../data"
resultfolder="../result"
suffix=".in"
for (( pe=16; pe<=128; pe=pe*2 )) do
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
    echo 'DEALING WITH '$filename >> ${resultname}
    for (( i = 1; i <= 3; i++ )); do
      echo 'RUN TH-'$i
      echo '######### Using Theory'$i' #########' >> ${resultname}
      echo `./run$i < $datafolder/$filename` >> ${resultname}
      echo '######### End #########' >> ${resultname}
    done
  done
done
echo 'LOADING RESULT'
echo `python3 loadresult.py`