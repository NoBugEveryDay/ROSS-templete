#include <stdio.h>

#include "ross.h"
#include "islip.h"

/**
 * Event cycle:
 * send- recv
 *       0.1 : 	MSG_GENERATE
 * 				↓
 * 0.1 - 0.2 : 	MSG_ARRIVE
 * 				↓
 * 0.2 - 0.3 : 	MSG_REQUEST
 * 
 *       0.4 :  MSG_GRANT_TRIGGER
 * 				↓
 * 0.4 - 0.5 : 	MSG_GRANT
 * 				↓
 * 0.5 - 0.9 :	MSG_CLEAN_GRANT
 * 
 *       0.6 :	MSG_ACCEPT_TRIGGER
 *				↓
 * 0.6 - 0.7 :	MSG_ACCEPT_PREPARE
 *              ↓
 * 0.7 - 0.8 :	MSG_ACCEPT
 * 
 *       0.9 :	MSG_CLEAN_PREPARE
 *              ↓
 * 0.9 - 0.95 :	MSG_CLEAN
*/

//Helper Functions
void SWAP (double *a, double *b) {
	double tmp = *a;
	*a = *b;
	*b = tmp;
}

void send_event(
	tw_lpid dest_gid, 
	tw_stime offset_ts, 
	tw_lp *sender,
	message_type msg_type,
	int content,	// -1 means nothing else need to transfer
	int history		// -1 means no history need to record
	) {
	
	tw_event *e = tw_event_new(dest_gid, offset_ts, sender);
	message *msg = tw_event_data(e);
	msg->sender = sender->id;
	msg->type = msg_type;
	msg->content = content;
	msg->history = history;
	tw_event_send(e);
}

void useless_lp_init(useless_lp_state *s, tw_lp *lp) {
	s->typeid = lp_type_dict[lp->gid];
	s->num_in_type = lp_num_in_type[lp->gid];
}

void inport_init(inport_state *s, tw_lp *lp) {
	s->typeid = lp_type_dict[lp->gid];
	s->num_in_type = lp_num_in_type[lp->gid];

	s->package_generated = 0;
	s->package_loss = 0;
	s->last_accept = -1;
	s->input_queue = (int*)malloc(sizeof(int)*switch_port_num);
	s->granter = (char*)malloc(sizeof(char)*switch_port_num);
	for (int i = 0; i < switch_port_num; i++) {
		s->input_queue[i] = 0;
		s->granter[i] = 0;
	}

	send_event(lp->gid, 0.1, lp, MSG_GENERATE, -1, -1);
	send_event(lp->gid, 0.6, lp, MSG_ACCEPT_TRIGGER, -1, s->last_accept);

	send_event(lp->gid, sampling_step-0.02, lp, MSG_SAMPLING_TRIGGER, find_type_lp[s->typeid][0], -1);
	if (lp->gid == find_type_lp[s->typeid][0]) {
		s->reduce = (long long*)malloc(sizeof(long long)*2);
		s->reduce[0] = 0; s->reduce[1] = 0;
		send_event(lp->gid, sampling_step, lp, MSG_SAMPLING, -1, -1);
	}
	else {
		s->reduce = NULL;
	}
}

void outport_init(outport_state *s, tw_lp *lp) {
	s->typeid = lp_type_dict[lp->gid];
	s->num_in_type = lp_num_in_type[lp->gid];

	s->last_grant = -1;
	s->requester = (int*)malloc(sizeof(int)*switch_port_num);
	for (int i = 0; i < switch_port_num; i++) {
		s->requester[i] = 0;
	}

	send_event(lp->gid, 0.4, lp, MSG_GRANT_TRIGGER, -1, -1);
}

void useless_lp_do_nothing(useless_lp_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {

}

void inport_event(inport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
	// FGN_STEP_IN_FUNC();
	// FGN_PRINT_INT("in_msg->type", in_msg->type);

	if (in_msg->type == MSG_GENERATE) {
		s->package_generated++;
		int dest_outport = tw_rand_integer(lp->rng, 0, switch_port_num-1);
		if (s->input_queue[dest_outport] == switch_input_buffer_size) {
			// To set s->package_loss++;
			send_event(lp->gid, 0.1, lp, MSG_ARRIVE, -1, -1);
		}
		else {
			// To set s->input_queue[dest_outport]++;
			send_event(lp->gid, 0.1, lp, MSG_ARRIVE, dest_outport, -1);
		}

		send_event(lp->gid, 1, lp, MSG_GENERATE, -1, -1);
	}
	else if (in_msg->type == MSG_ARRIVE) {
		int dest_outport = in_msg->content;

		if (dest_outport >= 0) {
			s->input_queue[dest_outport]++;
			int dest_gid = find_type_lp[OUTPORT_TYPE][dest_outport];
			send_event(dest_gid, 0.1, lp, MSG_REQUEST, s->num_in_type, -1);
		}
		else {
			s->package_loss++;
		}
	}
	else if (in_msg->type == MSG_GRANT) {
		int granter = in_msg->content;
		s->granter[granter] = 1;
		send_event(lp->gid, 0.4, lp, MSG_GRANT_CLEAN, granter, -1);
	}
	else if (in_msg->type == MSG_GRANT_CLEAN) {
		int granter = in_msg->content;
		s->granter[granter] = 0;
	}
	else if (in_msg->type == MSG_ACCEPT_TRIGGER) {
		int p, i;
		for (i = 0, p = (s->last_accept+1)%switch_port_num; i < switch_port_num; p = (p+1)%switch_port_num, i++) {
			if (s->granter[p]) {
				send_event(find_type_lp[OUTPORT_TYPE][p], 0.1, lp, MSG_ACCEPT_PREPARE, s->num_in_type, -1);
				send_event(lp->gid, 0.2, lp, MSG_ACCEPT, p, s->last_accept);
				break;
			}
		}
		
		send_event(lp->gid, 1, lp, MSG_ACCEPT_TRIGGER, -1, s->last_accept);
	}
	else if (in_msg->type == MSG_ACCEPT) {
		int accept_port = in_msg->content;
		s->input_queue[accept_port]--;
		s->last_accept = accept_port;
	}
	else if (in_msg->type == MSG_SAMPLING_TRIGGER) {
		int dest_gid = in_msg->content;
		send_event(dest_gid, 0.01, lp, MSG_SAMPLING_PREPARE, s->package_loss, s->package_generated);
		send_event(dest_gid, sampling_step, lp, MSG_SAMPLING_TRIGGER, dest_gid, -1);
	}
	else if (in_msg->type == MSG_SAMPLING_PREPARE) {
		s->reduce[0] += in_msg->content;
		s->reduce[1] += in_msg->history;
	}
	else if (in_msg->type == MSG_SAMPLING) {
		tw_output(lp, "Loss @ %lf : %lf%%\n", tw_now(lp), (double)s->reduce[0]/s->reduce[1]*100);
		send_event(lp->gid, 0.01, lp, MSG_SAMPLING_CLEAN, (double)s->reduce[0], s->reduce[1]);
		send_event(lp->gid, sampling_step, lp, MSG_SAMPLING, -1, -1);
	}
	else if (in_msg->type == MSG_SAMPLING_CLEAN) {
		s->reduce[0] = 0;
		s->reduce[1] = 0;
	}
	else {
		printf("Unhandeled forward message %d in inport_event\n", in_msg->type);
		exit(0);
	}

	// FGN_STEP_OUT_FUNC();
}

void inport_event_reverse(inport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
	// FGN_STEP_IN_FUNC();
	// FGN_PRINT_INT("in_msg->type", in_msg->type);

	if (in_msg->type == MSG_GENERATE) {
		s->package_generated--;
		tw_rand_reverse_unif(lp->rng);
	}
	else if (in_msg->type == MSG_ARRIVE) {
		if (in_msg->content >= 0) {
			int dest_outport = in_msg->content;
			s->input_queue[dest_outport]--;
		}
		else {
			s->package_loss--;
		}
	}else if (in_msg->type == MSG_GRANT) {
		int granter = in_msg->content;
		s->granter[granter] = 0;
	}
	else if (in_msg->type == MSG_GRANT_CLEAN) {
		int granter = in_msg->content;
		s->granter[granter] = 1;
	}
	else if (in_msg->type == MSG_ACCEPT_TRIGGER) {
		// nothing need to do
	}
	else if (in_msg->type == MSG_ACCEPT) {
		int accept_port = in_msg->content;
		s->input_queue[accept_port]++;
		s->last_accept = in_msg->history;
	}
	else if (in_msg->type == MSG_SAMPLING_TRIGGER) {
		// nothing need to do
	}
	else if (in_msg->type == MSG_SAMPLING_PREPARE) {
		s->reduce[0] -= in_msg->content;
		s->reduce[1] -= in_msg->history;
	}
	else if (in_msg->type == MSG_SAMPLING) {
		// nothing need to do
	}
	else if (in_msg->type == MSG_SAMPLING_CLEAN) {
		s->reduce[0] = in_msg->content;
		s->reduce[1] = in_msg->history;
	}
	else {
		printf("Unhandeled forward message %d in inport_event_reverse\n", in_msg->type);
		exit(0);
	}

	// FGN_STEP_OUT_FUNC();
}

void outport_event(outport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
	// FGN_STEP_IN_FUNC();
	// FGN_PRINT_INT("in_msg->type", in_msg->type);

	if (in_msg->type == MSG_REQUEST) {
		int src_inport = in_msg->content;
		s->requester[src_inport]++;
	}
	else if (in_msg->type == MSG_GRANT_TRIGGER) {
		int p, i;
		for (i = 0, p = (s->last_grant+1)%switch_port_num; i < switch_port_num; p = (p+1)%switch_port_num, i++) {
			if (s->requester[p]) {
				send_event(find_type_lp[INPORT_TYPE][p], 0.1, lp, MSG_GRANT, s->num_in_type, -1);
				break;
			}
		}
		send_event(lp->gid, 1, lp, MSG_GRANT_TRIGGER, -1, -1);
	}
	else if (in_msg->type == MSG_ACCEPT_PREPARE) {
		int src_inport = in_msg->content; 
		send_event(lp->gid, 0.1, lp, MSG_ACCEPT, src_inport, s->last_grant);
	}
	else if (in_msg->type == MSG_ACCEPT) {
		int src_inport = in_msg->content;
		s->requester[src_inport]--;
		s->last_grant = src_inport;
	}
	else {
		printf("Unhandeled forward message %d in type %d\n", in_msg->type, (int)s->typeid);
		exit(0);
	}

	// FGN_STEP_OUT_FUNC();
}

void outport_event_reverse(outport_state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
	// FGN_STEP_IN_FUNC();
	// FGN_PRINT_INT("in_msg->type", in_msg->type);

	if (in_msg->type == MSG_REQUEST) {
		int src_inport = in_msg->content;
		s->requester[src_inport]--;
	}
	else if (in_msg->type == MSG_GRANT_TRIGGER) {
		// Nothing need to do
	}
	else if (in_msg->type == MSG_ACCEPT_PREPARE) {
		// Nothing need to do
	}
	else if (in_msg->type == MSG_ACCEPT) {
		int src_inport = in_msg->content;
		s->requester[src_inport]++;
		s->last_grant = in_msg->history;
	}
	else {
		printf("Unhandeled forward message %d in type %d\n", in_msg->type, (int)s->typeid);
		exit(0);
	}

	// FGN_STEP_OUT_FUNC();
}

void inport_final(inport_state *s, tw_lp *lp) {
}

void outport_final(outport_state *s, tw_lp *lp) {
}

void inport_lp_sampling(inport_state *s, tw_bf *b, tw_lp *lp, void *sample) {
	// printf("Inport Sampling for %d @ %lf!\n", s->num_in_type, tw_now(lp));
	// fflush(stdout);
}

void inport_lp_sampling_reverse(inport_state *s, tw_bf *b, tw_lp *lp, void *sample) {
	// Nothing need to do
}