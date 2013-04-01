/* sleepy.h */

#ifndef SLEEPY_H_1727_INCLUDED
#define SLEEPY_H_1727_INCLUDED

/* Number of devices to create (default: sleepy0 and sleepy1) */
#ifndef SLEEPY_NDEVICES
#define SLEEPY_NDEVICES 10
#endif

#include <linux/wait.h>

/* The structure to represent 'sleepy' devices. 
 *  data - data buffer;
 *  buffer_size - size of the data buffer;
 *  block_size - maximum number of bytes that can be read or written 
 *    in one call;
 *  sleepy_mutex - a mutex to protect the fields of this structure;
 *  cdev - ñharacter device structure.
 *
 *  queue - wait_queue used to track processes waiting on this device;
 *  flag - used by sleeping threads to see if they should wake up.
 *         The process is as follows:
 *          1) The process going to sleep will save the current flag (atomically)
 *          2) When sleeping, the process will use "dev->flag != our_flag" as the condition,
 *             meaning that they will only wake up early if the flag has changed
 *          3) When a process wakes the sleepy device, its flag will be incremented, changing
 *             its value for all the sleeping processes to notice upon the subsequent wake_up
 *             call.
 *          This process is not infallable - it is known to fail at least in one pathological
 *          case: Let process A be sleeping on sleepy0, process C preempted while attempting
 *          to write to sleepy0, and process B currently reading from sleepy0 (effectively
 *          waking A). If process B proceeds to read 2**32 times, exactly, and then process C
 *          writing to sleepy0 before A has a chance to make its wake_up check, process A will
 *          not wake as intended. This case is, however, extremely unlikely.
 */
struct sleepy_dev {
  unsigned char *data;
  struct mutex sleepy_mutex; 
  struct cdev cdev;

  wait_queue_head_t queue;
  unsigned int flag;
};
#endif /* SLEEPY_H_1727_INCLUDED */
