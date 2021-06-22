#!/bin/bash

# This script is used to test the scalability of isilip
# It traverse from 32 ports to 4096 ports with from 1 process to 32 process
# Each test is run at least $RUN_TIME second

OUTPUT_FILE=scalability_test_result.dat
RUN_TIME=100

rm ${OUTPUT_FILE}

LOAD=90

for PORT_NUM in {32,64,128,256,512,1024,2048,4096}
do
    for PROCESS_NUM in {1,2,4,8,16,32}
    do
        for STEP in {512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,1048576,2097152,4194304,8388608,16777216,33554432}
        do
            echo -e "$PORT_NUM\t$PROCESS_NUM\t$STEP"
            START_TIME=$(date +%s%N)
            mpirun -n $PROCESS_NUM ../bin/islip \
                `# Basic Setting` \
                    --report-interval=0.01 `# report every 10%` \
                    --clock-rate=2700000000 `# Set CPU frequency for timming` \
                `# Tunning for speed` \
                    --synch=5 `# OPTIMISTIC_REALTIME=5` \
                    --nkp=4 `# num of kernel process in each process` \
                    --gvt-interval=100 `# the amount of real time in milliseconds between GVT computations for --synch=5` \
			        --max-opt-lookahead=1 `# maximum lookahead allowed in virtual clock time` \
                `# Simulation Setting` \
                    --end=$STEP `# End time for Simulation` \
                    --extramem=1000000 `# Enlarge it untill no extramem errors` \
                `# User parameter` \
                    --switch_port_num=$PORT_NUM \
                    --switch_load=0.$LOAD \
                `# End of parameter` \
            > /dev/null
            END_TIME=$(date +%s%N)
            COST_TIME=`echo "scale=3; ($END_TIME-$START_TIME)/1000000000" | bc`
            SPEED=`echo "scale=3; $STEP*$PORT_NUM/$COST_TIME" | bc`
            if [ $(echo "$COST_TIME > $RUN_TIME"|bc) = 1 ]
            then
                echo -e "$PORT_NUM\t$PROCESS_NUM\t$SPEED" >> ${OUTPUT_FILE}
                echo "COST_TIME=$COST_TIME > $RUN_TIME s, SPEED=$SPEED, PASS!"
                break
            else
                echo "COST_TIME=$COST_TIME <= $RUN_TIME s, SPEED=$SPEED, AGAIN!"
            fi
        done
    done
    echo >> ${OUTPUT_FILE}
done
