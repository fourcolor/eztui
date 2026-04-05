/* posix.c — POSIX terminal platform layer (termios) */

#define _POSIX_C_SOURCE 200809L
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "../src/term_internal.h"

static struct termios s_orig;
static volatile int   s_resize_flag;

static void handle_sigwinch(int sig) {
    (void)sig;
    s_resize_flag = 1;
}

int term_platform_init(void) {
    if (!isatty(STDIN_FILENO)) return -1;

    if (tcgetattr(STDIN_FILENO, &s_orig) < 0) return -1;

    struct termios raw = s_orig;
    /* Input: disable break signal, CR→NL, parity check, strip 8th bit,
     *        XON/XOFF flow control */
    raw.c_iflag &= ~(tcflag_t)(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* Output: disable post-processing */
    raw.c_oflag &= ~(tcflag_t)OPOST;
    /* Control: 8-bit chars */
    raw.c_cflag |= CS8;
    /* Local: disable echo, canonical mode, signals, extended processing */
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | IEXTEN | ISIG);
    /* read() returns after 1 byte, no timeout */
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) return -1;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigwinch;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, NULL);

    return 0;
}

void term_platform_cleanup(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &s_orig);
}

void term_platform_get_size(int *w, int *h) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *w = ws.ws_col;
        *h = ws.ws_row;
    } else {
        *w = 80;
        *h = 24;
    }
    s_resize_flag = 0;
}

float term_platform_cell_aspect(void) {
    /* First try TIOCGWINSZ pixel fields (fast, no round-trip). */
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0 && ws.ws_row > 0 &&
        ws.ws_xpixel > 0 && ws.ws_ypixel > 0) {
        float cell_w = (float)ws.ws_xpixel / ws.ws_col;
        float cell_h = (float)ws.ws_ypixel / ws.ws_row;
        return cell_h / cell_w;
    }

    /* Fallback: send \e[14t and parse \e[4;<ph>;<pw>t response. */
    if (write(STDOUT_FILENO, "\x1b[14t", 5) != 5)
        return 2.0f;

    char buf[64];
    int  len = 0;
    struct timeval tv = { .tv_sec = 0, .tv_usec = 500000 }; /* 500 ms */
    while (len < (int)sizeof(buf) - 1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) break;
        ssize_t n = read(STDIN_FILENO, buf + len, sizeof(buf) - 1 - (size_t)len);
        if (n <= 0) break;
        len += (int)n;
        buf[len] = '\0';
        /* Response ends with 't' */
        if (buf[len - 1] == 't') break;
    }

    /* Parse \e[4;<ph>;<pw>t */
    int ph = 0, pw = 0;
    if (len > 4 && buf[0] == '\x1b' && buf[1] == '[' &&
        sscanf(buf + 2, "4;%d;%dt", &ph, &pw) == 2 &&
        pw > 0 && ph > 0 &&
        ws.ws_col > 0 && ws.ws_row > 0) {
        float cell_w = (float)pw / ws.ws_col;
        float cell_h = (float)ph / ws.ws_row;
        return cell_h / cell_w;
    }

    return 2.0f; /* fallback: assume standard 1:2 cell */
}

int term_platform_read(char *buf, int buf_size, int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    if (s_resize_flag) {
        /* Signal resize by returning 0 bytes but caller checks flag via
         * term_platform_get_size(); we use a sentinel byte approach:
         * write a private resize byte into buf. */
        buf[0] = (char)0xFF; /* sentinel — handled in input.c */
        s_resize_flag = 0;
        return 1;
    }

    struct timeval tv, *tvp = NULL;
    if (timeout_ms >= 0) {
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tvp = &tv;
    }

    int r = select(STDIN_FILENO + 1, &fds, NULL, NULL, tvp);
    if (r <= 0) return r; /* 0=timeout, -1=error */

    ssize_t n = read(STDIN_FILENO, buf, (size_t)buf_size);
    return (n < 0) ? -1 : (int)n;
}
