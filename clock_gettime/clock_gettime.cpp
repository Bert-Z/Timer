#include <iostream>
#include <time.h>
using namespace std;
timespec diff(timespec start, timespec end);

int main()
{
    timespec time1, time2;
    clock_gettime(CLOCK_REALTIME, &time1);
    // func need to be timed
    clock_gettime(CLOCK_REALTIME, &time2);
    timespec t = diff(time1, time2);
    cout << t.tv_sec * 1000000000 + t.tv_nsec << " (ns)" << endl;
    return 0;
}

inline timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}