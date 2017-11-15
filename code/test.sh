datafolder="../data"
suffix=".in"
if [[ -f 'result.out' ]]; then
  rm 'result.out'
fi
for (( i = 1; i <= 3; i++ )); do
  echo '######### Using Theory'$i'#########' >> result.out
  for file in ${datafolder}/*${suffix}; do
    filename=`basename $file`
    echo 'Dealing with '$filename >> result.out
    echo `./run$i < $datafolder/$filename` >> result.out
    echo '' >> result.out
  done
done