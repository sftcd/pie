#/bin/bash

N=0
for nover8 in {1..32}
do
	((N=4*nover8))
	for limit in {128,160,192,256}
	do
		#echo $N,$limit
		./countem.sh $N $limit
	done
done

