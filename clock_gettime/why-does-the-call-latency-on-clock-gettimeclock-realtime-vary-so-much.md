I'm trying to time how long `clock_gettime(CLOCK_REALTIME,...)` takes to call. "Back in the day" I used to call it once at the top of a loop since it was a fairly expensive call. But now, I was hoping that with vDSO and some clock improvements, it might not be so slow.

I wrote some test code that used `__rdtscp` to time repeated calls to `clock_gettime` (the `rdtscp` calls went around a loop that called `clock_gettime` and added the results together, just so the compiler wouldn't optimize too much away).

If I call `clock_gettime()` in quick succession, the length of time goes from about 45k clock cycles down to 500 cycles. Some of this I thought could be contributed to the first call having to load the vDSO code (still doesn't full make sense to me), but how it takes a few calls to get 500 I cannot explain at all, and this behavior seems to be constant regardless of how I test it:

```
42467
1114
1077
496
455
```

However, if I sleep (for a second or ten, doesn't matter) between calls to clock_gettime, it only reaches a steady state of about 4.7k cycles:

Here at 10 second sleeps:

```
28293
1093
4729
4756
4736
```

Here at 1 second sleeps:

```
61578
855
4753
4741
5645
4753
4732
```

Cache behavior wouldn't seem to describe this (on a desktop system not doing much of anything). How much should I budget for a call to clock_gettime? Why does it get progressively faster to call? Why does sleeping a small amount of time matter so much?

**tl;dr** I'm trying to understand the time it takes to call `clock_gettime(CLOCK_REALTIME,...)` don't understand why it runs faster when called in quick succession as opposed to a second between calls.

**Update: Here's the cpuinfo on proc 0**

```
processor   : 0
vendor_id   : GenuineIntel
cpu family  : 6
model       : 158
model name  : Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz
stepping    : 9
microcode   : 0x84
cpu MHz     : 2800.000
cache size  : 6144 KB
physical id : 0
siblings    : 8
core id     : 0
cpu cores   : 4
apicid      : 0
initial apicid  : 0
fpu     : yes
fpu_exception   : yes
cpuid level : 22
wp      : yes
flags       : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb intel_pt tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid mpx rdseed adx smap clflushopt xsaveopt xsavec xgetbv1 xsaves dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp
bugs        :
bogomips    : 5616.00
clflush size    : 64
cache_alignment : 64
address sizes   : 39 bits physical, 48 bits virtual
power management:
```

Here is the recreated test code:

```c++
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>

// compiled gcc -Wall -O3 -o clockt clockt.cpp
// called glockt sleeptime trials loops

unsigned long long now() {
    struct timespec s;
    clock_gettime(CLOCK_REALTIME, &s);
    return (s.tv_sec * 1000000000ull) + s.tv_nsec;
}

int main(int argc, char **argv) {
    int sleeptime = atoi(argv[1]);
    int trials = atoi(argv[2]);
    int loops = atoi(argv[3]);

    unsigned long long x, y, n = 0;
    unsigned int d;


    x = __rdtscp(&d);
    n = now();
    asm volatile("": "+r" (n));
    y = __rdtscp(&d);

    printf("init run %lld\n", (y-x));

    for(int t = 0; t < trials; ++t) {
        if(sleeptime > 0) sleep(sleeptime);
        x = __rdtscp(&d);
        for(int l = 0; l < loops; ++l) {
            n = now();
            asm volatile("": "+r" (n));
        }
        y = __rdtscp(&d);
        printf("trial %d took %lld\n", t, (y-x));
    }

    exit(0);
}
```

# 回答1:

The very first time `clock_gettime` is called, a page fault occurs on the page that contains the instructions of that function. On my system, this is a soft page fault and it takes a few thousands of cycles to handle (up to 10,000 cycles). My CPU is running at 3.4GHz. I think that your CPU is running at a much lower frequency, so handling the page fault on your system would take more time. But the point here is that the first call to `clock_gettime` will take much more time than later calls, which is what you are observing.

The second major effect that your code is exhibiting is significant stalls due to instruction cache misses. It might appear that you are only calling two functions, namely `now` and `printf`, but these functions call other functions and they all compete on the L1 instruction cache. Overall, it depends on how all of these functions are aligned in the physical address space. When the sleep time is zero seconds, the stall time due to instruction cache misses is actually relatively small (you can measure this using the `ICACHE.IFETCH_STALL` performance counter). However, when the sleep time is larger than zero seconds, this stall time becomes significantly larger because the OS will schedule some other thread to run on the same core and that thread would different instructions and data. This explains why when you sleep, `clock_gettime` takes more time to execute.

Now regarding the second and later measurements. From the question:

```
42467
1114
1077
496
455
```

I have observed on my system that the second measurement is not necessarily larger than later measurements. I believe this is also true on your system. In fact, this seems to be the case when you sleep for 10 seconds or 1 second. In the outer loop, the two functions `now` and `printf` contain thousands of dynamic instructions and they also access the L1 data cache. The variability that you see between the second and later measurements is reproducible. So it's inherent in the functions themselves. Note that the execution time of `rdtscp` instruction itself may vary by 4 cycles. See also this.

In practice, the `clock_gettime` is useful when the desired precision is at most a million cycles. Otherwise, it can be misleading.





# 回答2:

I could not reproduce your results. Even with large sleep times (10 seconds), and a small number of loops (100), I always get timing of less than 100 clocks (less than 38 ns on my 2.6 GHz system).

For example:

```
./clockt 10 10 100
init run 14896
trial 0 took 8870 (88 cycles per call)
trial 1 took 8316 (83 cycles per call)
trial 2 took 8384 (83 cycles per call)
trial 3 took 8796 (87 cycles per call)
trial 4 took 9424 (94 cycles per call)
trial 5 took 9054 (90 cycles per call)
trial 6 took 8394 (83 cycles per call)
trial 7 took 8346 (83 cycles per call)
trial 8 took 8868 (88 cycles per call)
trial 9 took 8930 (89 cycles per call)
```

Outside of measurement or user-error (always the most likely reason), the most likely explanation is that your system is not using `rdtsc` as a time source, so a system call is made. You can configure the clock source explicitly yourself, or otherwise some heuristic is used which will select `rdtsc`-based `clock_gettime` only if it seems suitable on the current system.

The second most likely reason is that `clock_gettime(CLOCK_REALTIME)` doesn't go through the VDSO on your system, so is a system call even if `rdtsc` is ultimately being used. I guess that could be due to an old libc version or something like that.

The third most likely reason is that `rdtsc` on your system is slow, perhaps because it is virtualized or disabled on your system, and implement via a VM exit or OS trap.

# Single Loop Results

Trying with a single `clock_gettime` call per loop, I still get "fast" results after the first few trials. For example, `./clockt 0 20 1` gives:

```
init run 15932
trial 0 took 352 (352 cycles per call)
trial 1 took 196 (196 cycles per call)
trial 2 took 104 (104 cycles per call)
trial 3 took 104 (104 cycles per call)
trial 4 took 104 (104 cycles per call)
trial 5 took 104 (104 cycles per call)
trial 6 took 102 (102 cycles per call)
trial 7 took 104 (104 cycles per call)
...
```

Note that I made one modification to the test program, to print the time per call, which seems more useful than the total time. The `printf` line was modified to:

```c++
printf("trial %d took %lld (%lld cycles per call)\n", t, (y-x), (y-x)/loops);
```