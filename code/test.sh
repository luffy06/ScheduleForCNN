datafolder="../data"
resultfolder="../result"
suffix=".in"
for (( pe = 512; pe <= 512; pe=pe*2)); do
  if [[ -f 'config.in' ]]; then
    rm 'config.in'
  fi
  echo 'TOTAL_PE '$pe >> config.in
  echo 'PERIOD_TIMES 1000' >> config.in
  echo 'UPROUND 300' >> config.in
  for file in ${datafolder}/*${suffix}; do
    filename=`basename $file`
    echo 'Dealing with '$filename >> ${resultfolder}/result${pe}.out
    for (( i = 1; i <= 2; i++ )); do
      echo '######### Using Theory'$i' #########' >> ${resultfolder}/result${pe}.out
      echo `./run$i < $datafolder/$filename` >> ${resultfolder}/result${pe}.out
      echo '######### End #########' >> ${resultfolder}/result${pe}.out
    done
  done
done