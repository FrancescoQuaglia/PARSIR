# PARSIR

PARSIR (PARallel SImulation Runner) is a compile/runtime environment for discrete event 
simulation models written using the C programming language

The building of the application is based on the following API:

a) int ScheduleNewEvent(int destination, double timestamp, int event_type, char* body, int size)
which allows inserting an event for a destination simulation object, that will occur at a given
simulation time, into the PARSIR runtime

b) void ProcessEvent(unsigned int me, double now, int event_type, void *event_content, unsigned int size, void *ptr)
a callback that enables processing an event occurring at a given time in some specific 
simulation object - this callback is initially called with a runtime injected INIT event
occurring at simulation time zero in order to setup in memory the state of each simulation object
and to enable the initial scheduling of new events

You can explit the README file of the master branch to get additional information on
how to configure PARSIR and run it 

This specific branch implemens the NUMA-ubiquity facility where simulation objects are
transparently replicated on the different NUMA nodes of the underlying machine

To run this release please execute the following commands after moving to the "build" directory:
1) sudo make LKM - this command returns in output a line with the number of the memory-switch system call that has been
installed on Linux, use this number in the Makefile to configure the line "AFLAGS = -DSYSCALL_NUMBER=... "
To remove the LKM you can run "sudo make rm-LKM"

2) compile and run whatever of the available models (pcs or highway) by simply running "make <model-name>"
and then running the executable file located in the "bin" directory

The Makefile also specifies how you can confugure specific parameters for driving he execution
on your envirinment, like for example the number of NUMA nodes (called MEM_NODES) hosted by the platform

The "include" directory contains the file run.h which determines how many simulation objects will
belong to the model as well as how many threads will be started up by the PARSIR-simulator, 
and the lookahead of the simulation model to be executed. Currently they are specified as:
#define THREADS (10) 
#define OBJECTS (1024) 
#define LOOKAHEAD (1.0)

The include directory also holds other .h files where the specification of macros
for managing/building the PARSIR-simulator is defined, such as the maximum number of 
NUMA nodes to be managed, as well as the the maximum number of CPUs per NUMA node

Parallel execution of the simulation objects is carried out by PARSIR in a fully 
transparent manner to the application level code

REQUIREMENTS: PARSIR requires the numa library and its header to be installed on the system (it compiles with the -lnuma option) - for LKM it also requires installing the stuff available in the repository
https://github.com/FrancescoQuaglia/Linux-sys_call_table-discoverer




