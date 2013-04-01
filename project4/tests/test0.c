#include <stdio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main()
{
    struct timeval start;
    struct timeval end;
    int sleepyDescr;
    int sleepyTime = 5;
    char errMsg[1024];

    sleepyDescr = open("/dev/sleepy0", O_WRONLY);
    if (sleepyDescr == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }
    gettimeofday(&start, NULL);

    write(sleepyDescr, (void*)&sleepyTime, 4);

    gettimeofday(&end, NULL);

    printf("%d\n%d\n", (int)start.tv_sec, (int)end.tv_sec);

    return 0;
}
