#!/bin/bash

rm -rf out
mkdir -p out
for workload in varmail webserver fileserver;
do 
	for sched in noop deadline cfq grigora;
	do
	echo $sched > /sys/block/sdc/queue/scheduler
	for i in `seq 1 3`;
	do
		echo "starting filebench run $i $sched $workload"
		filebench << SCRIPT >> out/$sched-$workload
		load $workload
		set \$dir=/ssd/filebench
		run 60
SCRIPT
	done
	grep "IO Summary" out/$sched-$workload > out/$sched-$workload-results
	done
done

