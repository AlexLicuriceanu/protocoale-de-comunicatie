#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

using namespace std;

void fatal(char const *error_msg) {
    perror(error_msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    int source_fd = open(argv[1], O_RDONLY);
    int destination_fd = STDOUT_FILENO;
    char buffer[100];
    int bytes_count;

    if (source_fd < 0) {
        fatal("Error opening file.\n");
    }

    while ((bytes_count = read(source_fd, buffer, sizeof(buffer)))) {
        if (bytes_count < 0) {
            fatal("Error reading file.\n");
        }

        bytes_count = write(destination_fd, buffer, bytes_count);
        if (bytes_count < 0) {
            fatal("Error writing to stdout.\n");
        }
    }

    close(source_fd);
    return 0;
}