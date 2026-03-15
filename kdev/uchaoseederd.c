/* 
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT
 *
 *******************************************************************************
  @Grok: Write me a simple userspace deamon that can be started by /init and
  read 16 bytes from /dev/uchaos and ioctl() /dev/random crediting it as 256 bit
  of entropy (ratio 1:2, 4bits for byte). The deamon should obviously run in
  background and logging the least as possible in terms of not polluting the log.
  It takes three arguments: 1. time of reseeding with second granularity (min 1s,
  sleep(n) is fine and choose a reasonable default time); 2. reading char device
  (optional, default /dev/uchaos); 3. writing device (optional, default /dev/random).
  Add also a 4th parameter optional and default ignored for triggering a watchdog
  with the same frequency of the reseeding in this order: reseed sucessfully then
  trigger the whatch dog, failure or missing the reseeding fails to update it.
 *******************************************************************************
 *
 * Simple userspace daemon for early /init (initramfs).
 * Reads 16 bytes from /dev/uchaos (or custom device),
 * credits 256 bits of entropy to /dev/random via RNDADDENTROPY ioctl.
 *
 * Usage:
 *   uchaos-seed-daemon [interval_sec] [source_dev] [sink_dev] [watchdog_flag]
 *
 *   interval_sec   : reseed every N seconds (min 1, default 10)
 *   source_dev     : default /dev/uchaos
 *   sink_dev       : default /dev/random
 *   watchdog_flag  : any 4th argument enables watchdog on /dev/watchdog
 *                    (reseeds successfully → pet watchdog;
 *                     read or ioctl failure → do NOT pet)
 *
 * Compile:
 *   musl-gcc uchaoseederd.c -o uchaoseederd -static -s -Os -Wall
 *
 * Start from /init (example):
 *   /sbin/uchaos-seed-daemon 10 /dev/uchaos /dev/random 1 &
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_INTERVAL 10
#define ENTROPY_BYTES    16
#define CREDIT_BITS      256

static int source_fd = -1;
static int sink_fd   = -1;
static int wd_fd     = -1;

#ifdef _USE_KERNEL_HEADERS
#include <linux/random.h>
#include <linux/watchdog.h>
#else
#include <stdint.h>
#define RNDADDENTROPY    0x40085203 // _IOW('R', 3, int[32]) in linux/random.h
#define WDIOC_KEEPALIVE  0x8001 //     _IO('w', 1)           in linux/watchdog.h
typedef uint32_t __u32;
#endif

typedef struct {
    int entropy_count;
    int buf_size;
    __u32 buf[ENTROPY_BYTES / sizeof(__u32)];
} rand_pool_info_fixed;

static void do_reseed(void)
{
    char buf[ENTROPY_BYTES];
    ssize_t n = read(source_fd, buf, ENTROPY_BYTES);

    if (n != ENTROPY_BYTES) {
        return;                     /* silent failure */
    }

    rand_pool_info_fixed info = {
        .entropy_count = CREDIT_BITS,
        .buf_size      = ENTROPY_BYTES
    };
    memcpy(info.buf, buf, ENTROPY_BYTES);

    if (ioctl(sink_fd, RNDADDENTROPY, &info) == 0) {
        /* Success → pet watchdog if enabled */
        if (wd_fd >= 0) {
            ioctl(wd_fd, WDIOC_KEEPALIVE, NULL);
        }
    }
    /* ioctl failure or partial read → do nothing (no watchdog pet) */
}

int main(int argc, char **argv)
{
    int interval = DEFAULT_INTERVAL;
    const char *source = "/dev/uchaos";
    const char *sink   = "/dev/random";
    int enable_wd = 0;

    /* Argument parsing */
    if (argc > 1) {
        interval = atoi(argv[1]);
        if (interval < 1) interval = 1;
    }
    if (argc > 2) source = argv[2];
    if (argc > 3) sink   = argv[3];
    if (argc > 4) enable_wd = 1;          /* any 4th argument enables watchdog */

    /* Open source */
    source_fd = open(source, O_RDONLY | O_NONBLOCK);
    if (source_fd < 0) {
        fprintf(stderr, "uchaos-seed-daemon: cannot open %s: %s\n",
                source, strerror(errno));
        return 1;
    }

    /* Open sink */
    sink_fd = open(sink, O_RDWR);
    if (sink_fd < 0) {
        fprintf(stderr, "uchaos-seed-daemon: cannot open %s: %s\n",
                sink, strerror(errno));
        close(source_fd);
        return 1;
    }

    /* Optional watchdog */
    if (enable_wd) {
        wd_fd = open("/dev/watchdog", O_WRONLY);
        if (wd_fd < 0) {
            wd_fd = -1;                 /* silently disable if no watchdog */
        }
    }

    /* Daemonize (double fork + setsid) */
    if (fork() != 0) _exit(0);
    setsid();
    if (fork() != 0) _exit(0);

    /* Final cleanup */
    chdir("/");
    close(0); close(1); close(2);

    /* First reseed immediately */
    do_reseed();

    /* Main loop */
    while (1) {
        sleep((unsigned int)interval);
        do_reseed();
    }

    /* Never reached */
    return 0;
}
