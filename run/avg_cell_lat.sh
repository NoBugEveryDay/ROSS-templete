#!/bin/bash

# This script is used to calculate the average cell latency 
# under different port number and load.
# I'm sorry that this script can't get all the results.
# You have to adjust the parameters to get all the results.
# One of the biggest flaw of this product is that it counts average cell latency at the warmup stage,
# which make it hard to converge.

OUTPUT_FILE=avg_cell_lat_result.dat

# rm ${OUTPUT_FILE}

for PORT_NUM in {512,1024,2048,4096}
do
    if [[ $PORT_NUM -eq 16 || $PORT_NUM -eq 32 ]]
    then
        PROCESS_NUM=1
    else
        PROCESS_NUM=32
    fi

    if [ $PORT_NUM -le 256 ]
    then
        DIFF=0.0001
    else
        DIFF=0.001
    fi

    for LOAD in {21..99}
    do
        echo "PORT_NUM=$PORT_NUM LOAD=0.$LOAD"
        echo -e "$PORT_NUM\t$LOAD\t\c" >> ${OUTPUT_FILE}
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
                --end=200000000 `# End time for Simulation` \
                --extramem=1000000 `# Enlarge it untill no extramem errors` \
            `# User parameter` \
                --switch_port_num=$PORT_NUM \
                --switch_load=0.$LOAD \
                --sampling_step=100000 \
                --stop_diff=$DIFF \
            `# End of parameter` \
        | grep STABLE | cut -d : -f 2 >> ${OUTPUT_FILE}
    done
    echo >> ${OUTPUT_FILE}
done
