#ifndef _islip_h
#define _islip_h

#include "ross.h"

// defined in islip_main.c
extern tw_lpid global_lp_num_useful, global_lp_num_all;
extern unsigned int nlp_per_kp;
extern unsigned int switch_port_num;
extern unsigned int switch_input_buffer_size;
extern double sampling_step;

// defined in islip_map.c
extern tw_lpid *lp_type_dict;
extern int *lp_num_in_type;
extern tw_lpid **find_type_lp;

// Enumeration of lp type
typedef enum {
	USELESS_TYPE = 0,
	INPORT_TYPE = 1,
	OUTPORT_TYPE = 2
}lp_type_enum;

// Enumeration of message type
typedef enum {
	MSG_GENERATE,
	MSG_ARRIVE,
	MSG_REQUEST,
	MSG_GRANT_TRIGGER,
	MSG_GRANT,
	MSG_GRANT_CLEAN,
	MSG_ACCEPT_TRIGGER,
	MSG_ACCEPT_PREPARE,
	MSG_ACCEPT,
	MSG_SAMPLING_TRIGGER,
	MSG_SAMPLING_PREPARE,
	MSG_SAMPLING,
	MSG_SAMPLING_CLEAN
} message_type;

// Message struct
// this contains all data sent in an event
typedef struct {
	message_type type;
	tw_lpid sender;
	long long content;
	long long history;
} message;


//State struct
//   this defines the state of each LP
typedef struct {
	tw_lpid typeid;
	int num_in_type;
	long long package_generated, package_loss;
	int *input_queue;
	char *granter; // Outports who grant to this inport
	int last_accept;
	long long *reduce;
} inport_state;

typedef struct {
	tw_lpid typeid;
	int num_in_type;
	int last_grant;
	int *requester; // Inports who request this outport
} outport_state;

typedef struct {
	tw_lpid typeid;
	int num_in_type;
} useless_lp_state;

// Global variables used by both main and driver
// - this defines the LP types
extern tw_lptype islip_lp_types[];

// for tw_lptype islip_lp_types[]
void useless_lp_init(useless_lp_state *s, tw_lp *lp);
void useless_lp_do_nothing(useless_lp_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void inport_init(inport_state *s, tw_lp *lp);
void inport_event(inport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void inport_event_reverse(inport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void inport_final(inport_state *s, tw_lp *lp);
void outport_init(outport_state *s, tw_lp *lp);
void outport_event(outport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void outport_event_reverse(outport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp);
void outport_final(outport_state *s, tw_lp *lp);
tw_peid lp_gid_map_peid(tw_lpid gid);

// Custom mapping prototypes
void islip_map_init();
void islip_custom_initial_mapping(void);
tw_lp * lp_gid_map_lp_lid(tw_lpid lpid);
tw_lpid lp_gid_map_typeid (tw_lpid gid);

// for st_model_types islip_sampling_types[]
void inport_lp_sampling(inport_state *s, tw_bf *b, tw_lp *lp, void *sample);
void inport_lp_sampling_reverse(inport_state *s, tw_bf *b, tw_lp *lp, void *sample);

// Userful macro
#define YES1(a) ((a)?1:0)

#endif
