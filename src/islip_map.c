#include "islip.h"

// get nkp_per_pe from tw-setup.c
extern unsigned int nkp_per_pe;

/**
 * lp_type_dict[lp_gid]		:	The type of each lp. 
 * 								0 indicates the lp is useless.
 * lp_num_in_type[lp_gid]	:	The num of lp in its type
 * find_type_lp[type_id][i]	:	Find the i-th lp gid of type_id 
 */
tw_lpid *lp_type_dict;
int *lp_num_in_type;
tw_lpid **find_type_lp;

// Helper function for mapping. Execute in main()
void islip_map_init() {
	// Linear map and useless lp evenly distributed at the tail of on each kp and lp

	FGN_STEP_IN_FUNC();

	lp_type_dict = (tw_lpid*)malloc(sizeof(tw_lpid)*global_lp_num_all);
	lp_num_in_type = (int*)malloc(sizeof(int)*global_lp_num_all);

	tw_lpid useful_lp_i = 0;
	for (tw_peid pe_i = 0; pe_i < tw_nnodes(); pe_i++) {
		tw_lpid useful_lp_on_pe = global_lp_num_useful / tw_nnodes() + YES1(pe_i < global_lp_num_useful % tw_nnodes());
		for (tw_kpid kp_i = 0; kp_i < nkp_per_pe; kp_i++) {
			tw_kpid useful_lp_on_kp = useful_lp_on_pe / nkp_per_pe + YES1(kp_i < (useful_lp_on_pe % nkp_per_pe));
			for (tw_lpid lp_i = 0 ; lp_i < nlp_per_kp; lp_i++) {
				lp_type_dict[pe_i*nkp_per_pe*nlp_per_kp+kp_i*nlp_per_kp+lp_i] = YES1(lp_i < useful_lp_on_kp);
			}
		}
	}

	tw_lpid current_type = 1;
	int type_count[3] = {0, 0, 0};
	for (tw_lpid lp_i = 0; lp_i < global_lp_num_all; lp_i++) {
		if (lp_type_dict[lp_i]) {
			lp_type_dict[lp_i] = current_type;
			current_type = 3 - current_type;
		}
		lp_num_in_type[lp_i] = type_count[lp_type_dict[lp_i]];
		type_count[lp_type_dict[lp_i]]++;
	}

	int lp_type_num = 3;
	find_type_lp = (tw_lpid**)malloc(sizeof(tw_lpid*) * lp_type_num);
	for (int i = 0; i < lp_type_num; i++) {
		find_type_lp[i] = (tw_lpid*)malloc(sizeof(tw_lpid) * type_count[i]);
	}
	for (tw_lpid lp_i = 0; lp_i < global_lp_num_all; lp_i++) {
		find_type_lp[lp_type_dict[lp_i]][lp_num_in_type[lp_i]] = lp_i;
	}

	FGN_STEP_OUT_FUNC();
}

// Map from lp gid to typeid
tw_lpid lp_gid_map_typeid (tw_lpid gid) {
	return lp_type_dict[gid];
}

// Given a gid, return the local LP (global id => local id mapping)
tw_lp* lp_gid_map_lp_lid(tw_lpid gid){
  int local_id = gid - g_tw_lp_offset;
  return g_tw_lp[local_id];
}

// Map from gid to peid
tw_peid lp_gid_map_peid(tw_lpid gid){
	return (tw_peid) gid / g_tw_nlp;
}

// Bind lp to pe and kp, bind kp to lp
void islip_custom_initial_mapping(void){
	FGN_STEP_IN_FUNC();

	tw_lpid  lpid;
	tw_kpid  kpid;
	unsigned int j;

	if(!nlp_per_kp) {
		tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
	}

	g_tw_lp_offset = g_tw_mynode * g_tw_nlp;

	for(kpid = 0, lpid = 0; kpid < nkp_per_pe; kpid++) {
		tw_kp_onpe(kpid, g_tw_pe);

		for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++) {
			tw_lp_onpe(lpid, g_tw_pe, g_tw_lp_offset+lpid);
			tw_lp_onkp(g_tw_lp[lpid], g_tw_kp[kpid]);
		}
	}

	if(!g_tw_lp[g_tw_nlp-1]) {
		tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
	}

	if(g_tw_lp[g_tw_nlp-1]->gid != g_tw_lp_offset + g_tw_nlp - 1) {
		tw_error(TW_LOC, "LPs not sequentially enumerated!");
	}

	FGN_STEP_OUT_FUNC();
}