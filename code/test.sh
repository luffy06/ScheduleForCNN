datafolder="../data"
suffix=".in"
if [[ -f 'result.out' ]]; then
  rm 'result.out'
fi
for file in ${datafolder}/*${suffix}; do
  filename=`basename $file`
  echo 'Dealing with '$filename >> result.out
  for (( i = 1; i <= 3; i++ )); do
    echo '######### Using Theory'$i' #########' >> result.out
    echo `./run$i < $datafolder/$filename` >> result.out
    echo '######### End #########' >> result.out
  done
  echo '' >> result.out
done