#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#define NUM_PROCS 10
#define WAIT_TIME 65536

int sleepy(int sleepyId);

int main()
{
    int itr;
    int isSleepy;
    int isWaker;
    int sleepyDscr;
    char errMsg[1024];
    int sleepyPids[NUM_PROCS];
    int waitStatus;

    for (itr = 0; itr < NUM_PROCS; itr++)
    {
        sleepyPids[itr] = fork();
        if(sleepyPids[itr] == -1)
        {
            //FUUU - TODO, less of a hack.
            return 1;
        }
        else if(sleepyPids[itr] == 0)
        {
            isSleepy = itr+1;
            break;
        }
        else
            isSleepy = 0;
    }

    if(isSleepy)
        return sleepy(isSleepy);

    // Let's use system sleep, since we KNOW that works
    sleep(5);

    // Okay, now we'll wake everyone up
    sleepyDscr = open("/dev/sleepy0", O_RDONLY);
    if (sleepyDscr == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }
    read(sleepyDscr, NULL, 0);

    // Wait for each child to finish
    for (itr = 0; itr < NUM_PROCS; itr++)
    {
        if(-1 == waitpid(sleepyPids[itr], &waitStatus, 0))
        {
            if (strerror_r(errno, errMsg, 1024) == 0)
                printf("%s\n", errMsg);
        }
    }

    return 0;
}

int sleepy(int sleepyId)
{
    struct timeval start, end;
    int sleepyDscr;
    int sleepyTime = WAIT_TIME;
    char errMsg[1024];

    sleepyDscr = open("/dev/sleepy0", O_WRONLY);
    if (sleepyDscr == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }
    gettimeofday(&start, NULL);

    write(sleepyDscr, (void*)&sleepyTime, 4);

    gettimeofday(&end, NULL);

    printf("Proc %d -\n\t%d\n\t%d\n", sleepyId, (int)start.tv_sec, (int)end.tv_sec);

    return 0;
}
