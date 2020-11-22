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

#ifndef AM_OMPT_PROFILING_H
#define AM_OMPT_PROFILING_H

#ifdef PAPI_ENABLED
/*
 * Enables PAPI events to be traced
 * Additional options:
 * 		- AM_OMPT_PAPI_MAX_NUM_EVENTS sets the maximum possible number of user-defined events
 * 		- AM_OMPT_PAPI_EVENTS_ENV_VAR sets the env var for user to provide comma-separated events
 * 		- AM_OMPT_PAPI_MULTIPLEX_ENV_VAR sets the env var for user to enable multiplexing
 * 		- AM_OMPT_PAPI_MULTIPLEX provides the default multiplexing setting, if not user-provided
 * 		- AM_OMPT_PAPI_MINIMUM_ELAPSED_CYC sets the minimum cycles required before the counters can be read again
 */
#define AM_OMPT_PAPI_MAX_NUM_EVENTS 16
#define AM_OMPT_PAPI_EVENTS_ENV_VAR "OMPT_PAPI_EVENTS"
#define AM_OMPT_PAPI_MULTIPLEX_ENV_VAR "AM_OMPT_PAPI_MULTIPLEX"
#define AM_OMPT_PAPI_MULTIPLEX 0 /* Multiplexing disabled by default if env var unspecified */
#define AM_OMPT_PAPI_MINIMUM_ELAPSED_CYC 100000

/*
 * Each thread will have these fields in its thread_data structure
 * papi_count defines if counting is currently enabled
 * papi_reset is for user-application controlled pausing/resuming of event counting
 *
 */
#define AM_OMPT_PER_THREAD_PAPI_FIELDS \
	int papi_init; \
	int papi_count; \
	int papi_reset; \
	long long papi_counters[AM_OMPT_PAPI_MAX_NUM_EVENTS]; \
	long long papi_event_mapping[AM_OMPT_PAPI_MAX_NUM_EVENTS]; \
	int papi_event_set; \
	int papi_num_events; \
	long long last_read_timestamp; \

extern int papi_multiplex_enable;
extern int papi_num_events;
extern char* papi_event_names[AM_OMPT_PAPI_MAX_NUM_EVENTS];
int papi_multiplex_enable;
int papi_num_events;
char* papi_event_names[AM_OMPT_PAPI_MAX_NUM_EVENTS];

struct am_ompt_thread_data;

int am_ompt_setup_papi();
int am_ompt_init_papi_thread(struct am_ompt_thread_data* td);
int am_ompt_write_counter_descriptions_to_dsk(struct am_ompt_thread_data* td);
void am_ompt_trace_hardware_event_values(struct am_ompt_thread_data* td, long long timestamp);

#else

#define PER_THREAD_PAPI_FIELDS ;
#define am_ompt_trace_hardware_event_values(td, timestamp) do {} while(0)

#endif

#endif
