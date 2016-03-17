#!/bin/bash

OUT_DIR=fio_out
mkdir -p $OUT_DIR

for workload in varmail webserver fileserver;
do
	for sched in noop deadline cfq grigora;
	do
    fio --name=/ssd/test1 --direct=1 --size=512MB --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=8k --rwmixread=70 --iodepth=16 --numjobs=16 --runtime=60 --group_reporting --name=8k7030test1 > $OUT_DIR/$sched-$workload
    rm /tmp/test1
    fio --name=/ssd/test1 --direct=1 --size=512MB --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=8k --rwmixread=70 --iodepth=16 --numjobs=16 --runtime=60 --group_reporting --name=8k7030test2 >> $OUT_DIR/$sched-$workload
    rm /tmp/test1
    fio --name=/ssd/test1 --direct=1 --size=512MB --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=8k --rwmixread=70 --iodepth=16 --numjobs=16 --runtime=60 --group_reporting --name=8k7030test3 >> $OUT_DIR/$sched-$workload
    rm /tmp/test1

    echo "Ergebnisse: " > $OUT_DIR/$sched-$workload-results
    echo "" >> $OUT_DIR/$sched-$workload-results
    grep "READ" $OUT_DIR/$sched-$workload >> $OUT_DIR/$sched-$workload-results
    echo "" >> $OUT_DIR/$sched-$workload-results
    grep "WRITE" $OUT_DIR/$sched-$workload >> $OUT_DIR/$sched-$workload-results

  done
done
