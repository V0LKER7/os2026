#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define max_threads 4096

typedef struct {
    unsigned long long points;
    unsigned long long seed;
    unsigned long long hits;
    double radius;
} task_t;

static task_t tasks[max_threads];
static pthread_t threads[max_threads];

static void write_str(const char *s) {
    unsigned long len = 0;
    while (s[len]) len++;
    write(1, s, len);
}

static void write_u64(unsigned long long v) {
    char buf[24];
    int pos = 24;
    if (v == 0) buf[--pos] = '0';
    while (v > 0) {
        buf[--pos] = '0' + v % 10;
        v /= 10;
    }
    write(1, buf + pos, 24 - pos);
}

static void write_double(double v, int decimals) {
    unsigned long long scale = 1;
    for (int i = 0; i < decimals; i++) scale *= 10;
    unsigned long long scaled = (unsigned long long)(v * scale + 0.5);
    unsigned long long int_part = scaled / scale;
    unsigned long long frac_part = scaled % scale;
    write_u64(int_part);
    write_str(".");
    unsigned long long digit = scale / 10;
    while (digit > 0) {
        write_u64(frac_part / digit % 10);
        digit /= 10;
    }
}

static unsigned long long parse_u64(const char *s) {
    unsigned long long v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

static double parse_double(const char *s) {
    double v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    if (*s == '.') {
        s++;
        double step = 0.1;
        while (*s >= '0' && *s <= '9') {
            v += (*s++ - '0') * step;
            step /= 10;
        }
    }
    return v;
}

static int is_key(const char *s, const char *key) {
    while (*s && *key && *s == *key) {
        s++;
        key++;
    }
    return *s == 0 && *key == 0;
}

static double next_random(unsigned long long *state) {
    unsigned long long x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return (x >> 11) * (1.0 / 9007199254740992.0);
}

static void *worker(void *arg) {
    task_t *task = arg;
    unsigned long long state = task->seed;
    unsigned long long hits = 0;
    double r = task->radius;
    for (unsigned long long i = 0; i < task->points; i++) {
        double x = (2.0 * next_random(&state) - 1.0) * r;
        double y = (2.0 * next_random(&state) - 1.0) * r;
        if (x * x + y * y <= r * r) hits++;
    }
    task->hits = hits;
    return 0;
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static double estimate_area(double radius, unsigned long long total_points, int thread_count, int parallel) {
    unsigned long long base = total_points / thread_count;
    unsigned long long extra = total_points % thread_count;
    for (int i = 0; i < thread_count; i++) {
        tasks[i].points = base + (i < (int)extra ? 1 : 0);
        tasks[i].seed = (unsigned long long)(i + 1) * 0x9E3779B97F4A7C15ULL;
        tasks[i].hits = 0;
        tasks[i].radius = radius;
    }
    if (parallel) {
        for (int i = 0; i < thread_count; i++) pthread_create(&threads[i], 0, worker, &tasks[i]);
        for (int i = 0; i < thread_count; i++) pthread_join(threads[i], 0);
    } else {
        worker(&tasks[0]);
    }
    unsigned long long total_hits = 0;
    for (int i = 0; i < thread_count; i++) total_hits += tasks[i].hits;
    return 4.0 * radius * radius * total_hits / total_points;
}

int main(int argc, char **argv) {
    if (argc != 5 || !is_key(argv[3], "-t")) {
        write_str("usage: ./program <radius> <points> -t <threads>\n");
        return 1;
    }
    double radius = parse_double(argv[1]);
    unsigned long long total_points = parse_u64(argv[2]);
    int thread_count = (int)parse_u64(argv[4]);
    if (thread_count < 1 || thread_count > max_threads || total_points < (unsigned long long)thread_count) {
        write_str("bad arguments\n");
        return 1;
    }

    double start = now_ms();
    double area_seq = estimate_area(radius, total_points, 1, 0);
    double time_seq = now_ms() - start;

    start = now_ms();
    double area_par = estimate_area(radius, total_points, thread_count, 1);
    double time_par = now_ms() - start;

    double speedup = time_seq / time_par;
    double efficiency = speedup / thread_count;

    write_str("sequential area: ");
    write_double(area_seq, 4);
    write_str("\nparallel area:   ");
    write_double(area_par, 4);
    write_str("\nsequential ms:   ");
    write_double(time_seq, 3);
    write_str("\n\nthreads | time ms | speedup | efficiency\n");
    write_u64(thread_count);
    write_str(" | ");
    write_double(time_par, 3);
    write_str(" | ");
    write_double(speedup, 3);
    write_str(" | ");
    write_double(efficiency, 3);
    write_str("\n");
    return 0;
}