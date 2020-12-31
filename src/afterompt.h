/**
 * Copyright (C) 2018 Andi Drebes <andi@drebesium.org>
 * Copyright (C) 2019 Igor Wodiany <igor.wodiany@manchester.ac.uk>
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

#include <omp.h>
#include <ompt.h>

#define AM_OMPT_CHECK_WRITE(func_call)                                   \
  if (func_call) {                                                       \
    fprintf(stderr,                                                      \
            "Afterompt: Failed to write data to disk in %s\n"            \
            "           Consider increasing AFTERMATH_TRACE_BUFFER_SIZE" \
            " and AFTERMATH_EVENT_COLLECTION_BUFFER_SIZE\n",             \
            #func_call);                                                 \
    exit(1);                                                             \
  }

/* All function signatures as defined in OpenMP API Specification 5.0 */

/* Tool setup */
ompt_start_tool_result_t* ompt_start_tool(unsigned int omp_version,
                                          const char* runtime_version);

/* Tool initialization */
int ompt_initialize(ompt_function_lookup_t lookup, int num, ompt_data_t* data);

/* Initialize the data structures - this may be called by user application */
void initialize_tracing_data_structures();

/* Read excluded function addresses from environment */
int load_symbol_exclusions();

/* Tool clean up */
void ompt_finalize(ompt_data_t* data);

/* Callbacks */
void am_callback_thread_begin(ompt_thread_t type, ompt_data_t* data);

void am_callback_thread_end(ompt_data_t* data);

void am_callback_parallel_begin(ompt_data_t* task_data,
                                const ompt_frame_t* task_frame,
                                ompt_data_t* parallel_data,
                                unsigned int requested_parallelism, int flags,
                                const void* codeptr_ra);

void am_callback_parallel_end(ompt_data_t* parallel_data,
                              ompt_data_t* task_data, int flags,
                              const void* codeptr_ra);

void am_callback_task_create(ompt_data_t* task_data,
                             const ompt_frame_t* task_frame,
                             ompt_data_t* new_task_data, int flags,
                             int has_dependences, const void* codeptr_ra);

void am_callback_task_schedule(ompt_data_t* prior_task_data,
                               ompt_task_status_t prior_task_status,
                               ompt_data_t* next_task_data);

void am_callback_implicit_task(ompt_scope_endpoint_t endpoint,
                               ompt_data_t* parallel_data,
                               ompt_data_t task_data,
                               unsigned int actual_parallelism,
                               unsigned int index, int flags);

void am_callback_sync_region_wait(ompt_sync_region_t kind,
                                  ompt_scope_endpoint_t endpoint,
                                  ompt_data_t* parallel_data,
                                  ompt_data_t* task_data,
                                  const void* codeptr_ra);

void am_callback_mutex_released(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                                const void* codeptr_ra);

void am_callback_dependences(ompt_data_t* task_data,
                             const ompt_dependence_t* deps, int ndeps);

void am_callback_task_dependence(ompt_data_t* src_task_data,
                                 ompt_data_t* sink_task_data);

void am_callback_work(ompt_work_t wstype, ompt_scope_endpoint_t endpoint,
                      ompt_data_t* parallel_data, ompt_data_t* task_data,
                      uint64_t count, const void* codeptr_ra);

void am_callback_master(ompt_scope_endpoint_t endpoint,
                        ompt_data_t* parallel_data, ompt_data_t* taks_data,
                        const void* codeptr_ra);

void am_callback_sync_region(ompt_sync_region_t kind,
                             ompt_scope_endpoint_t endpoint,
                             ompt_data_t* parallel_data, ompt_data_t* task_data,
                             const void* codeptr_ra);

void am_callback_lock_init(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                           const void* codeptr_ra);

void am_callback_lock_destroy(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                              const void* codeptr_ra);

void am_callback_mutex_acquire(ompt_mutex_t kind, unsigned int hint,
                               unsigned int impl, ompt_wait_id_t wait_id,
                               const void* codeptr_pa);

void am_callback_mutex_acquired(ompt_mutex_t kind, ompt_wait_id_t wait_id,
                                const void* codeptr_ra);

void am_callback_nest_lock(ompt_scope_endpoint_t endpoint,
                           ompt_wait_id_t wait_id, const void* codeptr_ra);

void am_callback_flush(ompt_data_t* thread_data, const void* codeptr_ra);

void am_callback_cancel(ompt_data_t* task_data, int flags,
                        const void* codeptr_ra);

void am_callback_loop_begin(ompt_data_t* parallel_data, ompt_data_t* task_data,
                            int flags, int64_t lower_bound, int64_t upper_bound,
                            int64_t increment, int num_workers,
                            void* codeptr_ra);

void am_callback_loop_end(ompt_data_t* parallel_data, ompt_data_t* task_data);

void am_callback_loop_chunk(ompt_data_t* parallel_data, ompt_data_t* task_data,
                            int64_t lower_bound, int64_t upper_bound);

/* This must return a unique value for each pthread, for use by the PAPI library
 */
uint64_t am_ompt_thread_handle();

/* Function instrumentation calls:
 * May be inserted statically by compiler or dynamically (e.g. via Pin DBI)
 */

void am_function_entry(void* addr, int start_trace_signal);

void am_function_exit(void* addr, int stop_trace_signal);

/* Custom definitions of the compiler-inserted calls for function
*  instrumentation
*/

void __cyg_profile_func_enter(void *func,  void *caller)
   __attribute__((no_instrument_function));

void __cyg_profile_func_exit(void *func,  void *caller)
   __attribute__((no_instrument_function));
