# Timer

Timing function calls in a high precision manner



1.  clock_gettime()

   - easy to use

   - supporting clocks

     - **CLOCK_REALTIME**

       System-wide realtime clock. Setting this clock requires appropriate privileges.

     - **CLOCK_MONOTONIC**

       Clock that cannot be set and represents monotonic time since some unspecified starting point.

     - **CLOCK_PROCESS_CPUTIME_ID**

       High-resolution per-process timer from the CPU.

     - **CLOCK_THREAD_CPUTIME_ID**

       Thread-specific CPU-time clock.

   - overhead: 0.1ms~0.2ms

2.  rdtscp 

   - more accurate when working on the single process
   - overhead: less than 0.1ms

