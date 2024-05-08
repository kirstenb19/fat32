#include "shell.h"
#include "fat32.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Format should be: ./filesys <FAT32 image path>\n");
        return 1;
    }
    if (!mount_image(argv[1])) {
        printf("Failed to mount image. Exiting...\n");
        return 1;
    }

    run_shell();
    unmount_image();

    return 0;
}
