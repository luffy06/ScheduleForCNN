#!/bin/bash
# datafolder="../caffe_data"
# datafolder="../tensorflow_data"
datafolder="../extdata"
resultfolder="../result"
suffix=".in"
# algos=(ext algo lctes base)
algos=(ext algo lctes)
pes=(16 32 64 128)
period_times=(6000)
seg=' #################### '
for pt in ${period_times[*]}; do
  echo $seg'RUN TOTAL_ROUND:'$pt$seg
  for pe in ${pes[*]}; do
    echo $seg'CALC PE-'$pe$seg
    if [[ -f 'config.in' ]]; then
      rm 'config.in'
    fi
    resultname=${resultfolder}/${pe}.out
    if [[ -f ${resultname} ]]; then
      rm ${resultname}
    fi
    echo 'TOTAL_PE '$pe >> config.in
    echo 'PERIOD_TIMES '$pt >> config.in
    echo 'UPROUND 55' >> config.in
    for file in ${datafolder}/*${suffix}; do
      filename=`basename $file`
      echo $seg'GRAPH '$filename$seg
      for algo in ${algos[*]}; do
        date
        echo 'RUNNING '$algo
        echo 'RUNNING '$filename' '$algo >> ${resultname}
        echo `./run_$algo < $datafolder/$filename` >> ${resultname}
        date
      done
    done
  done
  echo $seg'LOADING RESULT'$seg
  echo `python3 loadresult.py`
  mv $resultfolder/output.xlsx $resultfolder/output_$pt.xlsx
done