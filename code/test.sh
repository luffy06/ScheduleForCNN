#!/bin/bash
# datafolder="../caffe_data"
datafolder="../tensorflow_data"
resultname="analyse.out"
suffix=".in"
seg=' #################### '
if [[ -f 'config.in' ]]; then
  rm 'config.in'
fi
if [[ -f $resultname ]]; then
  rm $resultname
fi
echo 'TOTAL_PE 16' >> config.in
echo 'PERIOD_TIMES 6000' >> config.in
echo 'UPROUND 300' >> config.in
for file in ${datafolder}/*${suffix}; do
  filename=`basename $file`
  echo $seg'GRAPH '$filename$seg
  echo 'RUNNING '$filename >> ${resultname}
  echo `./run_ana < $datafolder/$filename` >> ${resultname}
done