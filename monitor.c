
// signals
#define _POSIX_C_SOURCE 200809L

#include "cpu_info.h"       // proc_stat_t
#include "mem_info.h"       // proc_mem_t
#include "queue.h"          // queue_t
#include "graph.h"

#include <signal.h>         // sig_atomic_t, SIG*, sigaction, sigemptyset,
                            // sigaddset, sigprocmask, sigsuspend
#include <sys/time.h>       // setitimer
#include <unistd.h>         // STDOUT_FILENO

#include <stdio.h>          // *printf, perror
#include <stdlib.h>         // strtol, EXIT_*, NULL, malloc
#include <errno.h>          // errno
#include <stdarg.h>         // va_args
#include <string.h>         // memset, strcmp

#include <sys/ioctl.h>      // ioctl


#define SIZE(lst) \
    (sizeof(lst) / sizeof(*(lst)))


void error(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}


static const int term_sigs[] =
{
    SIGTERM, SIGINT, SIGABRT, SIGQUIT, SIGALRM,
};


volatile sig_atomic_t awoken = 0;
volatile sig_atomic_t termed = 0;


void handler(int sig, siginfo_t* info, void* ucontext)
{
    (void) info;
    (void) ucontext;

    if (sig == SIGALRM)
    {
        awoken = 1;
        return;
    }
    termed = 1;
}


void setup_term_signals()
{
    for (size_t i = 0; i < SIZE(term_sigs); i++)
    {
        struct sigaction act =
        {
            .sa_sigaction = handler,
            .sa_flags = SA_SIGINFO,
        };
        sigaction(term_sigs[i], &act, NULL);
    }
}


int setup_timer(long ms)
{
    struct itimerval interval =
    {
        .it_value = { .tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000 }
    };
    return setitimer(ITIMER_REAL, &interval, NULL);
}


typedef struct
{
    long s,
         ms;
} period_t;


int parse_num(long* num, const char* str, char** new_end)
{
    char* end;
    errno = 0;
    *num = strtol(str, &end, 10);
    int err = errno;
    if (err == ERANGE)
        return error("number too large\n"), -1;
    if (end == str || err != 0)
        return error("invalid number\n"), -1;
    if (new_end)
        *new_end = end;
    return 0;
}


// lol
int parse_period(period_t* d, char* arg)
{
    *d = (period_t){ 0 };

    long sec = 0;
    char* end = arg;
    if (parse_num(&sec, arg, &end) == -1)
        return -1;
    arg = end;

    d->s = sec;

    if (arg[0] == '\0')
        return 0;

    if (arg[0] == '.')
        arg += 1;
    else
        return -1;

    long ms = 0;
    if (parse_num(&ms, arg, NULL) == -1)
        return -1;

    while (ms < 1000)
        ms *= 10;
    ms /= 10;

    while (ms > 1000)
        ms /= 10;

    d->ms = ms;

    return 0;
}


typedef  void  (* function_t)  (void*);


int repeat(period_t period, function_t func, void* data)
{
    setup_term_signals();

    sigset_t set,
             old;
    sigemptyset(&set);

    for (size_t i = 0; i < SIZE(term_sigs); i++)
        sigaddset(&set, term_sigs[i]);

    if (sigprocmask(SIG_BLOCK, &set, &old) == -1)
        return perror("sigprocmask"), EXIT_FAILURE;

    while (1)
    {
        setup_timer(period.s * 1000 + period.ms);
        sigsuspend(&old);

        if (termed)
            break;

        func(data);
    }

    return EXIT_SUCCESS;
}


double read_percent(proc_stat_t* stat, proc_stat_t* prev_stat)
{
    proc_stat_read( stat, 1 );

    unsigned long long total = 
          ( proc_stat_total( stat ) - proc_stat_total( prev_stat ) );

    double frac = total == 0
        ? 0
        : ( (double) ( proc_stat_work( stat )
                     - proc_stat_work( prev_stat ) )
          / (double) total );

    *prev_stat = *stat;

    return frac * 100;
}


typedef struct
{
    proc_stat_t stat;
    proc_stat_t prev_stat;
    proc_mem_t memory;

    queue_t queue;

    int height;
    double max_value;
    double* values;
    int count;
    desc_t desc;

    int term_w;
    int term_h;


} monitor_t;


void copy(const queue_t* queue, double* values)
{
    for (unsigned i = 0; i < queue_capacity(queue); ++i)
        values[i] = queue_at(queue, i);
}


void monitor_clear(monitor_t* monitor)
{
    struct winsize win;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) != -1
            && (win.ws_row != monitor->term_h || win.ws_col != monitor->term_w))
    {
        monitor->term_w = win.ws_col;
        monitor->term_h = win.ws_row;
        printf("\033[2J" "\033[1;1H");
    }
    else
    {
        printf("\033[%dA\033[K", monitor->height + 3);
    }
    // printf("%dx%d", monitor->term_w, monitor->term_h);
}

void monitor_graph(monitor_t* monitor)
{
    copy(&monitor->queue, monitor->values);
    render_graph(monitor->values, monitor->count, monitor->max_value,
                monitor->height, &monitor->desc);
    fflush(stdout);
}


void monitor_cpu(void* ptr)
{
    monitor_t* monitor = (monitor_t*) ptr;
    double perc = read_percent(&monitor->stat, &monitor->prev_stat);
    queue_push(&monitor->queue, perc);

    monitor_clear(monitor);
    printf("CPU: %3.0lf\n", perc);
    monitor_graph(monitor);
}


void monitor_mem(void* ptr)
{
    monitor_t* monitor = (monitor_t*) ptr;
    if ( proc_mem_read( &monitor->memory ) == -1 ) {}
        // return 1;

    double filled = monitor->memory.total - monitor->memory.free;
    queue_push(&monitor->queue, filled);

    monitor_clear(monitor);
    printf("MEM: %12.0lf\n", filled);
    monitor_graph(monitor);
}


int main(int argc, char** argv)
{
    if (argc < 3)
        return error("Usage: %s ‹seconds› ‹cpu | mem›\n", argv[0]),
               EXIT_FAILURE;

    period_t period;
    if (parse_period(&period, argv[1]) == -1)
        return error("try e.g.: 1.5\n"), EXIT_FAILURE;

    monitor_t monitor = { 0 };
    monitor.count = 30;
    queue_init(&monitor.queue, monitor.count);
    monitor.values = (double*) malloc(monitor.count * sizeof(double));
    monitor.height = 7;

    for (int i = 0; i < monitor.height + 3; ++i)
        printf("\n");

    if (strcmp(argv[2], "cpu") == 0)
    {
        monitor.desc = (desc_t){ .align = 3, .decimals = 0 };
        monitor.max_value = 100;
        proc_stat_read(&monitor.prev_stat, 1);
        return repeat(period, monitor_cpu, &monitor);
    }

    if (strcmp(argv[2], "mem") == 0)
    {
        monitor.desc = (desc_t){ .align = 12, .decimals = 0 };
        proc_mem_read( &monitor.memory );
        monitor.max_value = monitor.memory.total;
        return repeat(period, monitor_mem, &monitor);
    }

    free(monitor.values);

    return error("Usage: %s ‹seconds› ‹cpu | mem›\n", argv[0]),
           EXIT_FAILURE;
}
