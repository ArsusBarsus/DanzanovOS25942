#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_LINES 1024

static char *mapped_data = NULL;
static size_t mapped_size = 0;

void alarm_handler(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\nTime is out! Full file content:\n", 33);
    if (mapped_data != NULL && mapped_size > 0) {
        write(STDOUT_FILENO, mapped_data, mapped_size);
    }
    _exit(0);
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "text.txt";

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    mapped_size = (size_t)st.st_size;
    if (mapped_size == 0) {
        fprintf(stderr, "File is empty\n");
        close(fd);
        return 1;
    }

    // Отображаем файл в память процесса
    mapped_data = (char *)mmap(NULL, mapped_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (mapped_data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    off_t offsets[MAX_LINES];
    size_t lengths[MAX_LINES];
    int line_count = 0;

    off_t current_start = 0;
    size_t current_len = 0;

    // Построение таблицы прямо по массиву в памяти
    for (size_t i = 0; i < mapped_size; i++) {
        if (mapped_data[i] == '\n') {
            if (line_count >= MAX_LINES) {
                fprintf(stderr, "Too many lines in file\n");
                break;
            }
            offsets[line_count] = current_start;
            lengths[line_count] = current_len;
            line_count++;

            current_start = (off_t)(i + 1);
            current_len = 0;
        } else {
            current_len++;
        }
    }

    if (current_len > 0 && line_count < MAX_LINES) {
        offsets[line_count] = current_start;
        lengths[line_count] = current_len;
        line_count++;
    }

    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        munmap(mapped_data, mapped_size);
        return 1;
    }

    int line_num;

    while (1) {
        printf("\nEnter line number within 5 seconds (0 to exit): ");
        fflush(stdout);

        alarm(5);
        int scan_res = scanf("%d", &line_num);
        alarm(0);

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
        printf("Line %d: %.*s\n", line_num, (int)lengths[idx], mapped_data + offsets[idx]);
    }

    munmap(mapped_data, mapped_size);
    return 0;
}
