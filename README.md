![Logo](/docs/afterompt.png)

AfterOMPT is a library that implements [OMPT](https://www.openmp.org/specifications/)
callbacks to collect dynamic events from OpenMP applications, and to write
them to a trace file using the [Aftermath](https://www.aftermath-tracing.com/)
tracing API.

## Download

The code can be simply downloaded from our git repository:

```
git clone https://github.com/pepperpots/Afterompt.git
```

## Dependencies

The only dependency for the project is [Aftermath](https://www.aftermath-tracing.com/).
The support for the OMPT events has not been merged into the main repository yet,
however the required code can be found in our development fork in [here](https://github.com/pepperpots/aftermath)
(branch: afterompt-support).

To enable loops tracing that relay on experimental callbacks modified versions of
Aftermath, LLVM OpenMP runtime and Clang are needed. All details can be found in
[this](https://github.com/IgWod/ompt-loops-tracing) repository.

## Configuration

Before running any commands please ensure that a required version of Aftermath
has been built. For the building instruction please refer to the
[official website](https://www.aftermath-tracing.com/prerelease/).

It is assumed that the OMPT compatible OpenMP runtime is already installed in the
system. If not runtime is present we recommend building and installing the
[LLVM OpenMP runtime](https://github.com/llvm/llvm-project) from source.

Now `CMAKE_PREFIX_PATH` has to be exported, so CMake can find required Aftermath
libraries.

```
export CMAKE_PREFIX_PATH="<path-to-aftermath>/install"
```

Now the Makefiles can be generated using following commands:

```
mkdir build
cd build/
cmake -DCMAKE_BUILD_TYPE=Release -DTRACE_X=TRUE ..
```

Where `-DTRACE_X=TRUE` enables a specific set of callbacks. Allowed
options are: `TRACE_LOOPS`, `TRACE_TASKS` and `TRACE_OTHERS`. Experimental
callbacks are enabled with `-DALLOW_EXPERIMENTAL=TRUE`. The scope of each
options is described later in this document.

Example CMake command:

```
cmake -DCMAKE_BUILD_TYPE=Release -DTRACE_TASKS=TURE -DTRACE_OTHERS=TRUE ..
```

## Build

Now the tool can be simply built by running the `make` command:

```
make install
```

The library is installed in the project's root inside the `install/` directory.

If the `ompt.h` header cannot be found by the compiler, its location should be
added to the include path:

```
export C_INCLUDE_PATH="/home/iwodiany/Projects/llvm-project/openmp/build/instal/usr/local/include"
```

## Usage

Now the tool can be used to trace any OpenMP applications, as long as, OMPT
compatible runtime is available in the system. The following instructions
outlines one of the possible ways to do it:

```
export AFTEROMPT_LIBRARY_PATH="<path-to-afterompt>/install"
export LD_LIBRARY_PATH="<path-to-openmp-runtime":${LD_LIBRARY_PATH}

source <path-to-aftermath>/env.sh

clang -o omp-program -fopenmp omp-program.c

AFTERMATH_TRACE_FILE=trace.ost \
LD_PRELOAD=${AFTEROMPT_LIBRARY_PATH}/libafterompt.so \
./omp-program
```

In the given example the tool is dynamically attached to the runtime by using
the `LD_PRELOAD` variable. `LD_LIBRARY_PATH` is not required, however it allows
the application to use the correct runtime, if multiple are available in the system,
or the search path has not been set when the runtime was installed.

## Available environmental variables

The following environmental variables can be exported to change specific
parameters of the library:

`AFTERMATH_TRACE_BUFFER_SIZE` (optional, default: 2^20) - Size of the trace wide
buffer in bytes.

`AFTERMATH_EVENT_COLLECTION_BUFFER_SIZE` (optional, default: 2^24) - Size of the per
core buffer in bytes.

`AFTERMATH_TRACE_FILE` (mandatory) - Name of the file where the data is written to.

## Available tracing information

Currently AfterOMPT traces the following states and events:

```
openmp_thread,
openmp_parallel,
openmp_task_create,
openmp_task_schedule,
openmp_implicit_task,
openmp_sync_region_wait,
openmp_mutex_released,
openmp_dependences,
openmp_task_dependence,
openmp_work,
openmp_master,
openmp_sync_region,
openmp_lock_init,
openmp_lock_destroy,
openmp_mutex_acquire,
openmp_mutex_acquired,
openmp_nest_lock,
openmp_flush,
openmp_cancel
```

Additional two are provided by experimental non-standard callbacks:

```
openmp_loop
openmp_loop_chunk
```

Detailed information about data attached to each state
and event can be found in the Aftermath types definitions.
Some extra information such as: loop and task instances, iteration
and task periods, etc. can be generated* when the trace is loaded
and processed in the Aftermath GUI.

(*) Requires experimental Aftermath from [here](https://github.com/IgWod/ompt-loops-tracing)

## Implemented callbacks

The following callbacks are implemented and compile-time
switches were provided to allow selective tracing of the events.

Always enabled:

* `ompt_callback_thread_begin`
* `ompt_callback_thread_end`

Enabled for `TRACE_LOOPS`:

* `ompt_callback_loop_begin`
* `ompt_callback_loop_end`
* `ompt_callback_loop_chunk`

(Currently to trace loops `ALLOW_EXPERIMENTAL` has
to be enabled as well, and the customized compiler
and runtime have to be installed)

Enabled for `TRACE_TASKS`:

* `ompt_callback_task_create`
* `ompt_callback_task_schedule`
* `ompt_callback_task_dependence`

Enabled for `TRACE_OTHERS`:

* `ompt_callback_parallel_begin`
* `ompt_callback_parallel_end`
* `ompt_callback_implicit_task`
* `ompt_callback_work`
* `ompt_callback_master`
* `ompt_callback_sync_region`
* `ompt_callback_sync_region_wait`
* `ompt_callback_mutex_released`
* `ompt_callback_dependences`
* `ompt_callback_lock_init`
* `ompt_callback_lock_destory`
* `ompt_callback_mutex_acquire`
* `ompt_callback_mutex_acquired`
* `ompt_callback_nest_lock`
* `ompt_callback_flush`
* `ompt_callback_cancel`

## Call-stack tracing

A further compile-time switch enables the library to expose
call-stack instrumentation functions for the target OpenMP program:

`SUPPORT_TRACE_CALLSTACK`

Once enabled, the library will support the tracing of function calls. For
instrumented functions, the trace will record the thread associated with the
call, the address of the function in the symbol table, and the interval that
the associated frame was on the stack.

Note: the library can support call-stack instrumentation, but does not
implement it. Implementing call-stack instrumentation may be done via (for
example):

* Compiling the target OpenMP program with compiler-inserted instrumentation
calls via `-finstrument-functions`. This is currently supported out-of-the-box.
* Inserting the instrumentation calls dynamically using
dynamic-binary-instrumentation. An Intel [PinTool](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html)
to implement this is forthcoming.

## Additional information

* Tracing of untied tasks has not been tested.

* One-to-one mapping between threads and cores is assumed, so that
  at most one worker thread can be attached to each core. Please
  refer to affinity settings of your runtime to ensure this behaviour.

## Supported software

The library was successfully used with following versions of the software:

| Operating System                       | Compiler                | Runtime      |
| -------------------------------------- | ----------------------- | ------------ |
| Ubuntu 18.04 LTS (Kernel version 5.3)  | Clang 9.0.0             | LLVM 9.0     |
| Ubuntu 18.04 LTS (Kernel version 4.15) | Clang 4.0.0             | Intel 19.1.1 |
|                                        | Intel C Compiler 19.1.1 |              |

## Publications

To cite this project please use the following BibTeX entry:

```
Work-in-progress
```

## Contributors

Igor Wodiany, Andi Drebes, Richard Neil, Antoniu Pop

For any problems or questions please contact Igor at
firstname.lastname@manchester.ac.uk.
