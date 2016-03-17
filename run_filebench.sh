#!/bin/bash

OUT_DIR=filebench_out

rm -rf $OUT_DIR
mkdir -p $OUT_DIR

echo "Ergebnisse: " > $OUT_DIR/gathered_results
for workload in varmail webserver fileserver;
do
	for sched in noop deadline cfq grigora;
	do
		echo $sched > /sys/block/sdc/queue/scheduler
		for i in `seq 1 10`;
		do
			echo "starting filebench run $i $sched $workload"
			filebench << SCRIPT >> $OUT_DIR/$sched-$workload
			load $workload
			set \$dir=/ssd/filebench
			run 60
SCRIPT
		done
	echo "$OUT_DIR/$sched-$workload" >> $OUT_DIR/gathered_results
	grep "IO Summary" $OUT_DIR/$sched-$workload | sed 's/mb#s,//'  | awk ' { ops += $7; rws += $11; anzahl ++; } END { print "runs: " anzahl ", OPS/S: " ops/anzahl ", rw/s: " rws/anzahl ; }' >> $OUT_DIR/gathered_results
	done
done
