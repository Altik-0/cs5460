// dev_access.c
// ------------
// 
// Small program designed to access various kernel modules written
// throughout the semester for cs5460 - Operating Systems - at the
// University of Utah, Spring 2013
//
// Author: Aric Parkinson
//------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

// Number of bytes in one MB
#define MEGABYTE 1048576
#define TEN_MB (MEGABYTE * 10)

// We want a long long (64 bit integer) to compute time difference in microsecs
typedef long long u64;

// Reads data from dev/input/mouse0 and prints the data to the screen
int read_mouse0();

// Times how long it takes to read TEN_MB from dev/urandom then write
// that byte to dev/null
int urandomToNull();

// Grabs a ticket from /dev/ticket (a custom kernel module) 10 times, sleeping
// for one second between reads
int getTickets();

int main(int argc, char** argv)
{
    // Validate command line args
    if (argc < 2)
    {
        printf("You must provide an argument to determine which operation to "
               "perform:\n\t0 - read from dev/input/mouse0 and print the read "
               "data to the screen.\n\t1 - time how long it takes to read 10MB "
               "from dev/urandom and then write it to dev/null.\n\t2 - TODO\n");
        return 1;
    }

    // Grab the integer. If sscanf returns anything other than 1, then the user's input
    // was invalid
    int option;
    int errCheck = sscanf(argv[1], "%d", &option);
    if (errCheck != 1)
    {
        printf("Your input must be a valid integer - 0, 1, or 2\n");
        return 1;
    }

    // Meaningful code: depending on which case we see from the user's input, we perform
    // a different action.
    switch(option)
    {
    case 0:
        errCheck = read_mouse0();
        break;
    case 1:
        errCheck = urandomToNull();
        break;
    case 2:
        errCheck = getTickets();
        break;
    // If we have any other value, it's outside of the valid range, so we error out
    default:
        printf("Your input must be between 0 and 2\n");
        return 1;
    }

    return errCheck;
}

int read_mouse0()
{
    // Used to check errors of system calls, and print error message
    int errCheck;
    char errMsg[1024];

    // Buffer to read mouse data into - real small, since we want to
    // read it one byte at a time
    char buf[1];

    // Open the mouse
    int mouseDscr = open("/dev/input/mouse0", O_RDONLY);
    if (mouseDscr == -1)
    {
        printf("Unable to open mouse0.\n");
        // strerror_r may fail, in which case we'll skip the error message.
        // Otherwise, we'll display it.
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    while(1)
    {
        // Attempt to read some data
        errCheck = read(mouseDscr, buf, 1);

        // Error check:
        if (errCheck == -1)
        {
            // Strerror_r may fail, in which case we skip the error message
            if (strerror_r(errno, errMsg, 1024) == 0)
                printf("%s\n", errMsg);
            return 1;
        }
        // No bytes were read this time, so we'll just try again
        if (errCheck == 0)
            continue;

        // Write out the byte
        printf("%d\n", buf[0]);
    }

    // Close the mouse
    errCheck = close(mouseDscr);
    if (errCheck == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    // everything went well, so return 0
    return 0;
}

int urandomToNull()
{
    // Used to check errors of system calls, and print error message
    int errCheck;
    char errMsg[1024];

    // Buffer to store bytes read from dev/urandom
    char* buf = malloc(TEN_MB);
    if (buf == NULL)
    {
        // If malloc failed, that pretty much means we're out of memory.
        // we're going to be as minimal as possible: puts instead of printf
        // because printf may use extra memory (obviously bad), then just
        // crash without any other work
        puts("Out of memory!");
        exit(1);
    }
    int bytesRead = 0;      // Total bytes read from dev/urandom
    int bytesWritten = 0;   // Total bytes written to dev/null

    // Timeval structs used for timing experiment
    struct timeval timer_start;
    struct timeval timer_stop;

    // Open ALL the buffers!
    int urandomDscr = open("/dev/urandom", O_RDONLY);
    if (urandomDscr == -1)
    {
        printf("Failed to open urandom\n");
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    int nullDscr = open("/dev/null", O_WRONLY);
    if (nullDscr == -1)
    {
        printf("Failed to open dev/null\n");
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    // Begin timer
    errCheck = gettimeofday(&timer_start, NULL);
    if (errCheck == -1)
    {
        printf("Timer start failed.\n");
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    // Read/Write until we're done:
    while (bytesRead < TEN_MB && bytesWritten < TEN_MB)
    {
        errCheck = read(urandomDscr, (buf + bytesRead), TEN_MB - bytesRead);
        if (errCheck == -1)
        {
            printf("Read error\n");
            if (strerror_r(errno, errMsg, 1024) == 0)
                printf("%s\n", errMsg);
            return 1;
        }

        // Read checked out, so adjust bytesRead amount
        bytesRead += errCheck;

        // We're trying to shave write calls, so if bytesRead - bytesWritten = 0,
        // may as well skip the call and continue to next read call
        if (bytesRead - bytesWritten == 0)
            continue;

        // We've got some stuff to write, so let's do that
        errCheck = write(nullDscr, (buf + bytesWritten), bytesRead - bytesWritten);
        if (errCheck == -1)
        {
            printf("Write error\n");
            if (strerror_r(errno, errMsg, 1024) == 0)
                printf("%s\n", errMsg);
            return 1;
        }
    }

    // End timer
    errCheck = gettimeofday(&timer_stop, NULL);
    if (errCheck == -1)
    {
        printf("Timer start failed.\n");
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    // Report results
    u64 usecStart = 1000000 * (u64)(timer_start.tv_sec) + (u64)(timer_start.tv_usec);
    u64 usecStop = 1000000 * (u64)(timer_stop.tv_sec) + (u64)(timer_stop.tv_usec);
    printf("Time taken:\t%lld usecs\n", usecStop - usecStart);

    // Close ALL the buffers!
    int retval = 0;
    
    errCheck = close(urandomDscr);
    if (errCheck == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        // We don't want to return, because we still need to make sure we at least
        // _try_ to close the other buffer
        retval = 1;
    }
    
    errCheck = close(nullDscr);
    if (errCheck == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        retval = 1;
    }

    return retval;
}

int getTickets()
{
    // Used to check errors of system calls, and print error message
    int errCheck;
    char errMsg[1024];

    // Open /dev/ticket0
    int ticketDscr = open("/dev/ticket0", O_RDONLY);
    if (ticketDscr == -1)
    {
        printf("Failed to open /dev/ticket0\n");
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    // Read the numbers:
    int i;
    for (i = 0; i < 10; i++)
    {
        int ticket_number;
        errCheck = read(ticketDscr, &ticket_number, 4);
        if (errCheck != 4)
            printf("Read number %d failed\n", i);
        else
            printf("Ticket number: %d\n", ticket_number);

        // wait one second before trying again
        sleep(1);
    }

    // Close /dev/ticket0
    errCheck = close(ticketDscr);
    if (errCheck == -1)
    {
        if (strerror_r(errno, errMsg, 1024) == 0)
            printf("%s\n", errMsg);
        return 1;
    }

    return 0;
}
