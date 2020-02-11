#include "timer.h"
#include <iostream>

int main()
{
    const auto cycles = timer::time([] {
        // func need to be timed
    });

    // cpu speed is 2099.996 MHz
    ulong count = cycles.count();
    std::cout << "It took " << count << " cycles, " << count / 2.099996 << " (ns)" << std::endl;
}