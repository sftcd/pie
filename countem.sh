#/bin/bash

# script to figure out how long it takes to get to knowing 50%
# of a hashed PIE crust

# parameters
# 256 bit sha-256 output is fixed
# 27 is the longest PIE that's shorter than a bare full hash
parcount=$#
if ((parcount==2))
then
	N=$1
	limit=$2
else
	N=8
	limit=128
fi

NotDone=true
iters=0

rm -f /tmp/set
touch /tmp/set

while ($NotDone) 
do
	./pie $N tcd.spki | grep decimal | sed -e 's/^rands.*://' > /tmp/thisrun
	OIFS=$IFS
	IFS=","
	rm -f /tmp/that
	for num in `cat /tmp/thisrun`
	do
		echo $num >>/tmp/that
	done
	rm -f /tmp/thisrun
	cat /tmp/that | sort -n > /tmp/thats
	rm -f /tmp/that

	cat /tmp/thats /tmp/set | sort -n | uniq > /tmp/newset
	mv /tmp/newset /tmp/set

	counters=`wc -l /tmp/set | awk '{print $1}'`

	if ((counters>=limit))
	then
		echo "iters,$iters,limit,$limit,N,$N,counters,$counters"
		exit
	fi
	((iters++))
	
	#echo "Done $iters"
	
done

