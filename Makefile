CC = mpiicc
CC_FLAGS = -O3 -xhost -std=c99
LD_FLAGS = -lROSS

BIN_DIR = bin
BUILD_DIR = build
INC_DIR = include
SRC_DIR = src
RUN_DIR = run

TARGET_PROGRAM = $(BIN_DIR)/islip

EXECUABLE = $(shell pwd)/$(TARGET_PROGRAM)
BUILD_OBJ = $(subst $(SRC_DIR), $(BUILD_DIR), $(subst .c,.o, $(wildcard $(SRC_DIR)/*.c)))
INCLUDE = -I./$(INC_DIR)

all : $(TARGET_PROGRAM)

$(BIN_DIR)/% : $(BUILD_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CC_FLAGS) $^ $(LD_FLAGS) -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -c $< $(INCLUDE) -o $@

clean :
	rm -r ./$(BUILD_DIR) ./$(BIN_DIR)

run : all
	@mkdir -p $(RUN_DIR)
	cd $(RUN_DIR) && \
	mpirun -n 32 $(EXECUABLE) \
		`# Basic Setting` \
			--report-interval=0.1 `# report every 10%` \
			--clock-rate=2700000000 `# Set CPU frequency for timming` \
		`# Tunning for speed` \
			--synch=5 `# OPTIMISTIC_REALTIME=5` \
			--nkp=4 `# num of kernel process in each process` \
			--gvt-interval=100 `# the amount of real time in milliseconds between GVT computations for --synch=5` \
			--max-opt-lookahead=1 `# maximum lookahead allowed in virtual clock time` \
		`# Simulation Setting` \
			--end=10000 `# End time for Simulation` \
			--extramem=1000000 `# Enlarge it untill no extramem errors` \
		`# User parameter` \
			--switch_port_num=1024 \
			--switch_load=0.5 \
			--sampling_step=1000 \
		`# End of parameter`