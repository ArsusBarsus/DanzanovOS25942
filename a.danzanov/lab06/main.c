#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_LINES 1024
#define MAX_LINE_LEN 1024

static int global_fd = -1;

void alarm_handler(int sig) {
    (void)sig;
    char buf[1024];
    ssize_t n;

    write(STDOUT_FILENO, "\nTime is out! Full file content:\n", 33);

    if (global_fd != -1 && lseek(global_fd, 0, SEEK_SET) != -1) {
        while ((n = read(global_fd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, n);
        }
        close(global_fd);
    }
    _exit(0);
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "text.txt";

    global_fd = open(filename, O_RDONLY);
    if (global_fd == -1) {
        perror("open");
        return 1;
    }

    off_t offsets[MAX_LINES];
    size_t lengths[MAX_LINES];
    int line_count = 0;

    char ch;
    off_t current_start = 0;
    size_t current_len = 0;
    ssize_t bytes_read;

    // Построение таблицы отступов и длин строк
    while ((bytes_read = read(global_fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            if (line_count >= MAX_LINES) {
                fprintf(stderr, "Too many lines in file\n");
                break;
            }
            offsets[line_count] = current_start;
            lengths[line_count] = current_len;
            line_count++;

            current_start = lseek(global_fd, 0L, SEEK_CUR);
            current_len = 0;
        } else {
            current_len++;
        }
    }

    if (bytes_read == -1) {
        perror("read");
        close(global_fd);
        return 1;
    }

    if (current_len > 0 && line_count < MAX_LINES) {
        offsets[line_count] = current_start;
        lengths[line_count] = current_len;
        line_count++;
    }

    // Регистрируем обработчик SIGALRM
    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        close(global_fd);
        return 1;
    }

    int line_num;
    char buf[MAX_LINE_LEN];

    while (1) {
        printf("\nEnter line number within 5 seconds (0 to exit): ");
        fflush(stdout);

        // Таймер на 5 секунд
        alarm(5);

        int scan_res = scanf("%d", &line_num);

        // Если успел, то отключаем

        if (scan_res != 1) {
            fprintf(stderr, "Invalid input\n");
            break;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > line_count) {
            printf("Line number out of range (1..%d)\n", line_count);
            continue;
        }

        int idx = line_num - 1;
        if (lengths[idx] >= sizeof(buf)) {
            fprintf(stderr, "Line is too long for buffer\n");
            continue;
        }

        if (lseek(global_fd, offsets[idx], SEEK_SET) == -1) {
            perror("lseek");
            break;
        }

        ssize_t n = read(global_fd, buf, lengths[idx]);
        if (n == -1) {
            perror("read line");
            break;
        }
        buf[n] = '\0';

        printf("Line %d: %s\n", line_num, buf);
    }

    close(global_fd);
    return 0;
}
