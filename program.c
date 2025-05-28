#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void process_input(const char *data) {
    if (strlen(data) < 5) return;

    if (strlen(data) > 0x8) return;

    if (data[0] == 'F') {
        if (data[1] == 'U') {
            if (data[2] == 'Z') {
                if (data[3] == 'Z') {
                    if (data[4] == '!') {
                        // Simulate a bug/crash
                        char cmd[0x20];
                        snprintf(cmd, 0x20 - 1, "echo %s", data);
                        system(cmd);
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    char buf[1024];
    int bytes_read = read(0, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        perror("read");
        return 1;
    }

    system("echo AAA");

    buf[bytes_read] = '\0';
    process_input(buf);
    return 0;
}