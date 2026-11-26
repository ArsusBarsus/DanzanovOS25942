#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAX_LINES 1024
#define MAX_LINE_LEN 1024

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "text.txt";

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
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

    // 1 Построение таблицы отступов и длин строк
    while ((bytes_read = read(fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            if (line_count >= MAX_LINES) {
                fprintf(stderr, "Too many lines in file\n");
                break;
            }
            offsets[line_count] = current_start;
            lengths[line_count] = current_len;
            line_count++;

            current_start = lseek(fd, 0L, SEEK_CUR);
            current_len = 0;
        } else {
            current_len++;
        }
    }

    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    if (current_len > 0 && line_count < MAX_LINES) {
        offsets[line_count] = current_start;
        lengths[line_count] = current_len;
        line_count++;
    }

    printf("Line table (total lines: %d):\n", line_count);
    for (int i = 0; i < line_count; i++) {
        printf("Line %d: offset = %ld, length = %zu\n",
               i + 1, (long)offsets[i], lengths[i]);
    }

    // 2 Поиск и вывод строк по номеру
    int line_num;
    char buf[MAX_LINE_LEN];

    while (1) {
        printf("\nEnter line number (0 to exit): ");
        if (scanf("%d", &line_num) != 1) {
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

        if (lseek(fd, offsets[idx], SEEK_SET) == -1) {
            perror("lseek");
            break;
        }

        ssize_t n = read(fd, buf, lengths[idx]);
        if (n == -1) {
            perror("read line");
            break;
        }
        buf[n] = '\0';

        printf("Line %d: %s\n", line_num, buf);
    }

    close(fd);
    return 0;
}
