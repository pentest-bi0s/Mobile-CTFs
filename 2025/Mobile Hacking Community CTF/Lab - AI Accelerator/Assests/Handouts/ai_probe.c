/* ai_probe.c
 * Safe test harness for /dev/ai_accel (non-exploit).
 * - Opens device
 * - Performs controlled reads/writes
 * - Issues ioctls in a bounded/random fashion
 *
 * Compile for aarch64 and run on the target.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

void usage(const char *p) {
    fprintf(stderr, "Usage: %s <device> [iters] [max_size]\n", p);
    fprintf(stderr, "Example: %s /dev/ai_accel 100 1024\n", p);
}

int main(int argc, char **argv) {
    const char *dev = (argc > 1) ? argv[1] : "/dev/ai_accel";
    int iters = (argc > 2) ? atoi(argv[2]) : 200;
    int max_size = (argc > 3) ? atoi(argv[3]) : 512;

    if (iters <= 0 || max_size < 0) { usage(argv[0]); return 1; }

    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("[+] opened %s (fd=%d)\n", dev, fd);

    char *buf = malloc((max_size > 0) ? max_size : 1);
    if (!buf) {
        perror("malloc");
        close(fd);
        return 1;
    }

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    for (int i = 0; i < iters; ++i) {
        int op = rand() % 3;
        int len = (max_size > 0) ? (rand() % (max_size + 1)) : 0;

        if (op == 0) {
            /* write */
            memset(buf, 'A' + (i % 26), len);
            ssize_t w = write(fd, buf, len);
            if (w < 0) {
                printf("[iter %d] write(%d) => -1 errno=%d (%s)\n", i, len, errno, strerror(errno));
            } else {
                printf("[iter %d] write(%d) => %zd\n", i, len, w);
            }
        } else if (op == 1) {
            /* read */
            ssize_t r = read(fd, buf, len);
            if (r < 0) {
                printf("[iter %d] read(%d) => -1 errno=%d (%s)\n", i, len, errno, strerror(errno));
            } else {
                printf("[iter %d] read(%d) => %zd\n", i, len, r);
            }
        } else {
            /* ioctl: bounded/randomized - uses safe range only */
            unsigned long cmd = 0x40000000UL | (rand() & 0x00FFFFFFUL);
            int rc = ioctl(fd, cmd, NULL);
            if (rc < 0) {
                if ((i % 50) == 0)
                    printf("[iter %d] ioctl(0x%lx) => -1 errno=%d (%s)\n", i, cmd, errno, strerror(errno));
            } else {
                printf("[iter %d] ioctl(0x%lx) => %d\n", i, cmd, rc);
            }
        }
        /* small delay to avoid overwhelming device; remove only if you know what you're doing */
        usleep(10000);
    }

    free(buf);
    close(fd);
    printf("[+] done\n");
    return 0;
}