#!/bin/bash

rm -rf out
mkdir -p out
for workload in varmail webserver fileserver;
do 
	for sched in noop grigora deadline cfq;
	do
	echo $sched > /sys/block/vda/queue/scheduler
	for i in `seq 1 3`;
	do
		echo "starting filebench run $i $sched $workload"
		filebench << SCRIPT >> out/$sched-$workload
		set \$dir=/tmp
		load $workload
		run 600
SCRIPT
	done
	grep "IO Summary" $sched-$workload > $sched-$workload-results
	done
done

