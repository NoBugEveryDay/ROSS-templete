#!/bin/bash

# This script is used to test the correctness of islip
# The result can be compared with Nick McKeown's result

OUTPUT_FILE=corrent_verify_result.dat

rm ${OUTPUT_FILE}
echo -e "x\ty" >> ${OUTPUT_FILE}

PORT_NUM=16
PROCESS_NUM=1

for LOAD in {21..99}
do
	echo "--switch_load=0.$LOAD"
	echo -e "$LOAD\t\c" >> ${OUTPUT_FILE}
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
			--end=20000000 `# End time for Simulation` \
			--extramem=1000000 `# Enlarge it untill no extramem errors` \
		`# User parameter` \
			--switch_port_num=$PORT_NUM \
			--switch_load=0.$LOAD \
			--sampling_step=100000 \
			--stop_diff=0.0001 \
		`# End of parameter` \
	| grep STABLE | cut -d : -f 2 >> ${OUTPUT_FILE}
done
