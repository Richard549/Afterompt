/**
 * Copyright (C) 2020 Richard Neill <richard.neill@manchester.ac.uk>
 *
 * Afterompt is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef PAPI_ENABLED

#include "profiling.h"
#include "trace.h"
#include "afterompt.h"

#include <aftermath/core/on_disk_write_to_buffer.h>

#include "papi.h"
#include <string.h>

int am_ompt_load_papi_env(){

	const char* env_papi_events = getenv(AM_OMPT_PAPI_EVENTS_ENV_VAR);
	if(env_papi_events == NULL || env_papi_events[0] == '\0'){
		fprintf(stderr,"AfterOMPT hardware event profiling enabled but %s was undefined or empty.\n",AM_OMPT_PAPI_EVENTS_ENV_VAR);
		return 1;
	}

	// Copy the events from the environment as strtok mutates in place
	char papi_events_str[strlen(env_papi_events)];
	strcpy(&papi_events_str[0],env_papi_events);

	// Parse each event into the global config variable papi_event_names
	// User input events may be invalid, but deferring error to PAPI library later
	char* event;
	char* delimiter = ",";
	int event_index = -1;

	event = strtok(papi_events_str, delimiter);
	while(event != NULL){
		event_index++;

		if(event_index >= AM_OMPT_PAPI_MAX_NUM_EVENTS){
			fprintf(stderr,"Number of PAPI events exceeded limit (%d).\n",AM_OMPT_PAPI_MAX_NUM_EVENTS);
			return 1;
		}
		
		papi_event_names[event_index] = malloc(strlen(event)*sizeof(char));
		
		if(papi_event_names[event_index] == NULL){
			fprintf(stdout,"Failed to allocate memory when parsing PAPI events from environment.\n");
			return 1;
		}

		strcpy(papi_event_names[event_index],event);
		event = strtok(NULL, delimiter);
	}
	
	papi_num_events = event_index + 1;

	const char* env_papi_multiplex = getenv(AM_OMPT_PAPI_MULTIPLEX_ENV_VAR);
	if(env_papi_multiplex == NULL || env_papi_multiplex[0] == '\0'){
		papi_multiplex_enable = AM_OMPT_PAPI_MULTIPLEX;
	} else if(env_papi_multiplex[0] == '0'){
		papi_multiplex_enable = 0;
	} else if(env_papi_multiplex[0] == '1'){
		papi_multiplex_enable = 1;
	}

	return 0;
}

int am_ompt_write_counter_descriptions_to_dsk(struct am_ompt_thread_data* td){

	for(int i = 0; i < td->papi_num_events; i++) {
	
		struct am_dsk_counter_description dsk_cd;

		dsk_cd.counter_id = td->papi_event_mapping[i];
		dsk_cd.name.str = papi_event_names[i];
		dsk_cd.name.len = strlen(papi_event_names[i]);

		AM_OMPT_CHECK_WRITE(am_dsk_counter_description_write_to_buffer_defid(&am_ompt_trace.data, &dsk_cd));
		fprintf(stdout, "Wrote %s to disk.\n", dsk_cd.name.str);
	}
	
	return 0;
}

int am_ompt_setup_papi(){

	if(am_ompt_load_papi_env())
		return 1;

	int retval = PAPI_library_init(PAPI_VER_CURRENT);

	if (retval != PAPI_VER_CURRENT && retval > 0) {
		fprintf(stderr,"AfterOMPT: PAPI library version mismatch.\n");
		return 1;
	}

	if (retval < 0) {
		fprintf(stderr,"AfterOMPT could not init PAPI library: %s (%d) %d!\n", PAPI_strerror(retval), retval, PAPI_VER_CURRENT);
		return 1;
	}

	if (PAPI_is_initialized() != PAPI_LOW_LEVEL_INITED) {
		fprintf(stderr, "AfterOMPT could not init PAPI library (low-level part)!\n");
		return 1;
	}

	if ((retval = PAPI_thread_init(am_ompt_thread_handle)) != PAPI_OK) {
		fprintf(stderr, "AfterOMPT could not init PAPI thread support: %s\n", PAPI_strerror(retval));
		return 1;
	}

	if(papi_multiplex_enable){
		if (PAPI_multiplex_init() != PAPI_OK) {
			fprintf(stderr, "AfterOMPT could not init PAPI multiplexing!\n");
			return 1;
		}
	}

	return 0;

}

/* Must be called from the relevant thread's own context */
int am_ompt_init_papi_thread(struct am_ompt_thread_data* td){

	if(papi_num_events == 0){
		// If we aren't tracing any events, don't bother initialising
		td->papi_count = 0;
		td->papi_num_events = 0;
		return 0;
	}

	int err, i;

	memset(td->papi_counters, 0, sizeof(td->papi_counters));
	td->papi_count = 0;
	td->papi_reset = 1;
	td->last_read_timestamp = 0;

	PAPI_register_thread();

	td->papi_event_set = PAPI_NULL;

	/* Create event set */
	if ((err = PAPI_create_eventset(&td->papi_event_set)) != PAPI_OK) {
		fprintf(stderr, "AfterOMPT could not create PAPI event set for thread %lu: %s!\n", td->tid, PAPI_strerror(err));
		return 1;
	}

	/* Assign CPU component */
	if ((err = PAPI_assign_eventset_component(td->papi_event_set, 0)) != PAPI_OK) {
		fprintf(stderr, "AfterOMPT could not assign PAPI event set to component 0 for thread %lu: %s\n", td->tid, PAPI_strerror(err));
		return 1;
	}

	if(papi_multiplex_enable){
		/* Enable multiplexing */
		if((err = PAPI_set_multiplex(td->papi_event_set)) != PAPI_OK) {
			fprintf(stderr, "AfterOMPT could not enable PAPI multiplexing for event set: %s!\n", PAPI_strerror(err));
			return 1;
		}
	}

	td->papi_num_events = papi_num_events;

	for(i = 0; i < papi_num_events; i++) {
		td->papi_event_mapping[i] = i;

		if((err = PAPI_add_named_event(td->papi_event_set, papi_event_names[i])) != PAPI_OK) {
			fprintf(stderr, "AfterOMPT could not add event \"%s\" to PAPI event set: %s!\n", papi_event_names[i], PAPI_strerror(err));
			return 1;
		}
	}

	if(papi_num_events > 0){
		if((err = PAPI_start(td->papi_event_set)) != PAPI_OK) {
			fprintf(stderr, "AfterOMPT could not start PAPI counters on thread %lu: %s!\n", td->tid, PAPI_strerror(err));
			return 1;
		}   
		td->papi_count = 1;
	}

	td->papi_init = 1;
	return 0;
}

uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void am_ompt_trace_hardware_event_values(struct am_ompt_thread_data* td, long long timestamp){

	//uint64_t pre_time_1 = rdtsc();

	if(!td->papi_count)
		return;

	if(td->papi_reset){
		int err;
		if ((err = PAPI_reset(td->papi_event_set)) != PAPI_OK) {
			fprintf(stderr, "Could not reset counters: %s!\n", PAPI_strerror(err));
			exit(1);
		}
		td->papi_reset = 0;
	}

	if(td->papi_num_events > 0) {

		uint64_t time_since_last_sample = (timestamp - td->last_read_timestamp);
		//fprintf(stdout,"Time since last sample: %llu\n", time_since_last_sample);
		if(time_since_last_sample <= AM_OMPT_PAPI_MINIMUM_ELAPSED_CYC)
			return;

		// Read the counter values
		if(PAPI_accum(td->papi_event_set, td->papi_counters) != PAPI_OK) {
			fprintf(stderr, "Could not read counters for thread %lu\n", td->tid);
			exit(1);
		}

		struct am_buffered_event_collection* c = td->event_collection;

		//uint64_t pre_time_2 = rdtsc();
		// Write the counter values
		for(int i = 0; i < td->papi_num_events; i++){
			struct am_dsk_counter_event e = {c->id, td->papi_event_mapping[i], timestamp, td->papi_counters[i]};
			AM_OMPT_CHECK_WRITE(am_dsk_counter_event_write_to_buffer_defid(&c->data, &e));
		}
		//uint64_t post_time_2 = rdtsc();
		//fprintf(stdout,"Time spent in writing counts to disk:%llu\n", post_time_2 - pre_time_2);

		td->last_read_timestamp = timestamp;

	}
	//uint64_t post_time_1 = rdtsc();
	//fprintf(stdout,"Time spent in trace_hardware_event_values function:%llu\n", post_time_1 - pre_time_1);
	
}

#endif
