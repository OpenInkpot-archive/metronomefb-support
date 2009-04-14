/*
 * metronometempd -- maintaining proper temperature info in metronome eink
 * controller
 * © 2009 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Remove this hack as soon as glibc 2.9 is in IP
 * #include <sys/timerfd.h>
 */
#include <sys/syscall.h>
#include <time.h>
#define TFD_TIMER_ABSTIME (1 << 0)

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 64
#define DEFAULT_PERIOD 10

char* source_file;
char* destination_file;
int coefficient;

size_t lread(int fh, char* buf, size_t bufsize)
{
    int count = bufsize;
    while(count)
    {
        int res = read(fh, buf, count);
        if(res == 0)
            break;
        if(res == -1 && (errno == EINTR || errno == EAGAIN))
            continue;
        if(res > 0)
        {
            buf += res;
            count -= res;
            continue;
        }

        return -1;
    }
    return bufsize - count;
}

size_t lwrite(int fh, const char* buf, size_t count)
{
    while(count)
    {
        int res = write(fh, buf, count);
        if(res == -1 && (errno == EINTR || errno == EAGAIN))
            continue;
        if(res > 0)
        {
            buf += res;
            count -= res;
            continue;
        }

        return -1;
    }
    return 0;
}

void update_temp(const char* src, const char* dest, int coeff)
{
    int srcfd = open(src, O_RDONLY);
    if(srcfd == -1)
        return;

    int destfd = open(dest, O_WRONLY);
    if(destfd == -1)
        goto err2;

    char buf[BUFSIZE];
    int read_ = lread(srcfd, buf, BUFSIZE-1);
    if(read_ <= 0)
        goto err1;
    buf[read_] = '\0';

    int64_t t;
    if(1 != sscanf(buf, "%jd", &t))
        goto err1;

    t /= coeff;

    snprintf(buf, BUFSIZE, "%jd", t);

    lwrite(destfd, buf, strlen(buf));

err1:
    close(destfd);
err2:
    close(srcfd);
}

int run(const char* src, const char* dest, int coeff, int period)
{
    /* Remove this hack as soon as glibc 2.9 is in IP
     * int tfd = timerfd_create(CLOCK_REALTIME, 0);
     */
    int tfd = syscall(SYS_timerfd_create, CLOCK_REALTIME, 0);

    if(tfd == -1)
    {
        perror("timerfd_create");
        return EXIT_FAILURE;
    }

    struct itimerspec timer = {
        .it_value =
        {
            .tv_sec = time(NULL),
        },
        .it_interval = {
            .tv_sec = period,
        }
    };

    /* Remove this hack as soon as glibc 2.9 is in IP
     * if(-1 == timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timer, NULL))
     */
    if(-1 == syscall(SYS_timerfd_settime, tfd, TFD_TIMER_ABSTIME, &timer, NULL))
    {
        perror("timerfd_settime");
        return EXIT_FAILURE;
    }

    if(-1 == daemon(0, 0))
    {
        perror("daemon");
        return EXIT_FAILURE;
    }

    for(;;)
    {
        uint64_t exp;
        int res = read(tfd, &exp, sizeof(uint64_t));
        if(res == -1 && errno == EAGAIN)
            continue;
        if(res == -1 && errno == EINTR)
            continue;
        if(res != sizeof(uint64_t))
            exit(EXIT_FAILURE);

        update_temp(src, dest, coeff);
    }
}

void usage()
{
    fprintf(stderr, "Usage: metronometempd <sensor> <controller> <coefficient> [<period>]\n");
    fprintf(stderr, "Maintains temperature information in metronome e-ink controller.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "<sensor> is a status file for temperature sensor\n");
    fprintf(stderr, " (e.g. /sys/class/hwmon/hwmon0/device/temp1_input)\n");
    fprintf(stderr, "<controller> is a waveform control file\n");
    fprintf(stderr, " (e.g. /sys/class/graphics/fb1/temp");
    fprintf(stderr, "<coefficient> is a temperature conversion coefficient, e.g. 1000\n");
    fprintf(stderr, "<period> is an period between updates, defaults to 10s\n");
}

int main(int argc, char** argv)
{
    if(argc < 4 || argc > 5)
    {
        usage();
        exit(EXIT_FAILURE);
    }

    if(argv[1][0] == '-' || argv[2][0] == '-' || argv[3][0] == '-')
    {
        usage();
        exit(EXIT_FAILURE);
    }

    int period = DEFAULT_PERIOD;
    if(argc == 5)
        period = atoi(argv[4]);

    return run(argv[1], argv[2], atoi(argv[3]), period);
}
