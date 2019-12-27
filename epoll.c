#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

//#define GPIO_PATH "/proc/sys/kernel/hostname"
#define GPIO_PATH "/proc/sys/mod/test/action"

#define USLEEP(usec)

int
main(int argc, const char **argv)
{
        int fds;
        int i;
        int flag;
        static char buf[256];


        char val;
        struct pollfd pfd;
        int rv;

        fds = open(GPIO_PATH, O_RDWR);

        rv = read(fds, &val, 1);
        if ( rv != 1 ) {
                return 255;
        }

        fprintf(stderr, "wait...");

        pfd.fd = fds;
        pfd.events = POLLPRI | POLLERR;
// ここで POLLIN を付け加えないこと。
// 付け加えると、read 可能なのですぐに poll が正常終了する
// これは Linux の Documentation/gpio.txt にあるとおり。
        pfd.revents = 0;
        poll(&pfd, 1, -1);

// 必要なら read する。
// すでに読んでいるので lseek する
        lseek(fds, 0, SEEK_SET);
        rv = read(fds, &val, 1);
        close(fds);

        if ( rv != 1 ) {
                return 254;
        }
        printf("%c\n", val);

        return rv;
}
