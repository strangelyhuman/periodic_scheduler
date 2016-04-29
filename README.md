Periodic Scheduler
==================

This repository contains a periodic scheduler that has been implemented as a Kernel Module 
using KThreads for the Linux Kernel v4.5.

Concept
-------

A Periodic Scheduler is used by an Operating System to ensure that the Tasks complete within
a predefined deadline. The basic idea behind periodic scheduling is this - to check if a task
completes within the period it is defined for. If the tasks finish executing, then it is
scheduled to run again for the same period. If it fails to complete, then the Scheduler halts
execution.

Here, KThreads are used to implement threads of execution for periods of 1hz, 10hz and 100hz
and timers. These timers on counting down, trigger callbacks that check a shared bitfield for
Program Completion. If the Bitfield is set, then it signifies that a Program has finished 
execution and the Callback releases a Semaphore. The Task, which is sleeping, because it is waiting
on that semaphore, obtains it and starts execution. If the task fails to complete, that is in case of
an overrun, then the bitfield will not be set, which when observed by the Timer Callback, will result 
in it halting execution. The bitfield is also monitored by a separate thread, which will initiate a cleanup
when a task overruns.
