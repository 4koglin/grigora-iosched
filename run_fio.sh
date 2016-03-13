
fio --name=/tmp/test1 --direct=1 --size=1GB --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=8k --rwmixread=70 --iodepth=16 --numjobs=16 --runtime=60 --group_reporting --name=8k7030test1 > resultfio
fio --name=/tmp/test2 --direct=1 --size=1GB --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=8k --rwmixread=70 --iodepth=16 --numjobs=16 --runtime=60 --group_reporting --name=8k7030test2 >> resultfio
fio --name=/tmp/test3 --direct=1 --size=1GB --rw=randrw --refill_buffers --norandommap --randrepeat=0 --ioengine=libaio --bs=8k --rwmixread=70 --iodepth=16 --numjobs=16 --runtime=60 --group_reporting --name=8k7030test3 >> resultfio

rm /tmp/test*

echo "Ergebnisse: " > results
echo "" >> results
grep "READ" ./resultfio >> results
echo "" >> results
grep "WRITE" ./resultfio >> results
