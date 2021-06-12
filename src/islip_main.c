#include "ross.h"
#include "islip.h"

// IMPORTANT: The following variables are set in order in program.

/**
 * tw_nnodes()  :   MPI_Comm_size
 * g_tw_mynode  :   MPI_Comm_rank
 */

/*
 *  nkp_per_pe	:	Access nkp_per_pe which is defined in tw-setup.c
 *					It should be used after tw_init()
 */
extern unsigned int nkp_per_pe;

/**
 * global_lp_num_useful	:	The number of real useful lp in all process
 * global_lp_num_all	:	The number of lp in all process.
 * 							There are some useless empty lp among them
 * 							because the lp in each pe and kp should be the saame
 */
tw_lpid global_lp_num_useful, global_lp_num_all;

/*
 * nlp_per_kp  :   The num of lp in each kp
 *					Set in main()
 */
unsigned int nlp_per_kp;

/**
 * Some variables in ross-global.h
 * g_tw_nlp         :   LP num on this PE.
 * 						It was set in tw_define_lps()
 * g_tw_nkp         :   The num of kp on each pe.
 * 						The same as nkp_per_pe, but it was init in tw_define_lps()
 * g_tw_lp          :   Public LP object array (on this processor)
 * g_tw_kp          :   Public KP object array (on this processor)
 * g_tw_lp_offset   :   global id of g_tw_lp[0] (on this processor)
 * 						It was set in mapping procedure
 */

// Define LP types
//   these are the functions called by ROSS for each LP
//   multiple sets can be defined (for multiple LP types)
tw_lptype islip_lp_types[] = {
	{ // Useless lp
		(init_f)	useless_lp_init,
		(pre_run_f)	NULL,
		(event_f)	NULL,
		(revent_f)	NULL,
		(commit_f)	NULL,
		(final_f)	NULL,
		(map_f)		lp_gid_map_peid,
		sizeof(useless_lp_state)
	},
	{ // inport lp
		(init_f)	inport_init,
		(pre_run_f)	NULL,
		(event_f)	inport_event,
		(revent_f)	inport_event_reverse,
		(commit_f)	NULL,
		(final_f)	inport_final,
		(map_f)		lp_gid_map_peid,
		sizeof(inport_state)
	},
	{ // outport lp
		(init_f)	outport_init,
		(pre_run_f)	NULL,
		(event_f)	outport_event,
		(revent_f)	outport_event_reverse,
		(commit_f)	NULL,
		(final_f)	outport_final,
		(map_f)		lp_gid_map_peid,
		sizeof(outport_state)
	}
};

// Define sampling_types
//   these are the functions called by ROSS when need to sampling
//   It should be consistent with islip_lp_types
st_model_types islip_sampling_types[] = {
	{ // Useless lp tw_lptype
		// For event tracing
		(ev_trace_f)		NULL,	// function pointer to collect data about events for given LP
		(size_t)			NULL,	// size of data collected from model for each event
		// For real time or GVT-based sampling
		(model_stat_f)		NULL,	// real time or GVT-based sampling
		(size_t)			NULL,	// size of data collected from model at sampling points
		// For virtual time sampling
		(sample_event_f)	NULL,	// function pointer for virtual time sampling of model data
		(sample_revent_f)	NULL,	// RC function pointer for virtual time sampling of model data
		(size_t)			NULL	// size of data collected using virtual time sampling
	},
	{ // inport lp
		(ev_trace_f)		NULL,
		(size_t)			NULL,
		(model_stat_f)		NULL,
		(size_t)			NULL,
		(sample_event_f)	inport_lp_sampling,
		(sample_revent_f)	inport_lp_sampling_reverse, // This can not be NULL, otherwise errors will occur
		(size_t)			1		// This have to be non-zero, otherwise sampling will not be called
	},
	{ // outport lp
		(ev_trace_f)		NULL,
		(size_t)			NULL,
		(model_stat_f)		NULL,
		(size_t)			NULL,
		(sample_event_f)	NULL,
		(sample_revent_f)	NULL,
		(size_t)			NULL
	}
};

//Define command line arguments default values
unsigned int switch_port_num = 16;
double switch_load = 0.9;
double sampling_step = 0;

//add your command line opts
const tw_optdef model_opts[] = {
	TWOPT_GROUP("ROSS Model"),
	TWOPT_UINT("switch_port_num", switch_port_num, "Port num of simulated switch"),
	TWOPT_DOUBLE("switch_load", switch_load, "The load of simulated switch"),
	TWOPT_DOUBLE("sampling_step", sampling_step, "Sampling every sampling_step virtual time. sampling_step<=0 means do NOT sampling"),
	TWOPT_END(),
};


//for doxygen
#define model_main main

int model_main (int argc, char* argv[]) {
	int i;
	int num_lps_per_pe;

	tw_opt_add(model_opts);
	tw_init(&argc, &argv);
	FGN_STEP_IN_FUNC();

	global_lp_num_useful = switch_port_num * 2;
	if (global_lp_num_useful % (tw_nnodes() * nkp_per_pe)) {
		// global_lp_num_all must be a multiple of tw_nnodes() * nkp_per_pe
		global_lp_num_all = (1 + global_lp_num_useful / (tw_nnodes() * nkp_per_pe)) * (tw_nnodes() * nkp_per_pe);
	}
	else {
		global_lp_num_all = global_lp_num_useful;
	}
	num_lps_per_pe = global_lp_num_all / tw_nnodes();
	nlp_per_kp = num_lps_per_pe / nkp_per_pe;

	FGN_PRINT_INT64("global_lp_num_useful", global_lp_num_useful);
	FGN_PRINT_INT64("global_lp_num_all", global_lp_num_all);
	FGN_PRINT_INT("num_lps_per_pe", num_lps_per_pe);
	FGN_PRINT_INT("nkp_per_pe", nkp_per_pe);
	FGN_PRINT_INT("nlp_per_kp", nlp_per_kp);

	// Set Custom Mapping
	islip_map_init();
	g_tw_mapping = CUSTOM;
	g_tw_custom_initial_mapping = &islip_custom_initial_mapping;
	g_tw_custom_lp_global_to_local_map = &lp_gid_map_lp_lid;	


	// Set up LPs within ROSS
	tw_define_lps(num_lps_per_pe, sizeof(message));
	// note that g_tw_nlp gets set here by tw_define_lps

	// Tell ROSS how to map lp_gid -> lptype index
	g_tw_lp_typemap = &lp_gid_map_typeid;

	// Tell ROSS the type of lp
	g_tw_lp_types = islip_lp_types;

	// Tell ROSS the sampling type of lp
	g_st_model_types = islip_sampling_types;
	tw_lp_setup_types();

	// Check Mapping
	if (0) {
		FGN_MPI_CHECK() {
			for (tw_lpid lp_i = 0; lp_i < global_lp_num_all; lp_i++) {
				printf("lp_gid:%llu\t", lp_i);
				printf("type_id:%llu\t", (unsigned long long)(lp_type_dict[lp_i]));
				printf("num_in_type:%lld\t", (unsigned long long)(lp_num_in_type[lp_i]));
				printf("\n");fflush(stdout);
			}
		}

		for (tw_lpid lp_i = 0; lp_i < g_tw_nlp; lp_i++) {
			printf("lp_gid:%llu\t", (unsigned long long)(g_tw_lp[lp_i]->gid));
			printf("lp_gid:%llu\t", (unsigned long long)(lp_type_dict[g_tw_lp[lp_i]->gid]));
			printf("pe_id:%llu\t", (unsigned long long)(g_tw_lp[lp_i]->pe->id));
			printf("kp_id:%llu\t", (unsigned long long)(g_tw_lp[lp_i]->kp->id));
			printf("\n");fflush(stdout);
		}
	}

	tw_run();
	
	tw_end();

	FGN_STEP_OUT_FUNC();

	return 0;
}
