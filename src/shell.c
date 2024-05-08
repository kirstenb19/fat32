#include "shell.h"
#include "fat32.h"
#include <stdio.h>
#include <string.h>

void run_shell() {
    char command[256];

    while (1) {
        printf("%s%s> ", imageName, currentPath);
        fgets(command, sizeof(command), stdin);

        // trailing space
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "exit") == 0) 
        {
            printf("Exiting......\n");
            unmount_image();
            break;
        } 
        else if (strcmp(command, "info") == 0) 
        {
            print_boot_sector_info();
        } 
        else if (strncmp(command, "cd ", 3) == 0) 
        {
            change_directory(command + 3);
        } 
        else if (strcmp(command, "ls") == 0) 
        {
            list_directory();
        } 
        else if (strncmp(command, "mkdir ", 6) == 0) 
        {
            mkdir(command + 6);
        } 
        else if (strncmp(command, "creat ", 6) == 0) 
        {
            creat(command + 6);
        }

        else if (strncmp(command, "open ", 5) == 0)
        {
            char *filename = strtok(command + 5, " ");
            char *mode = strtok(NULL, " ");
            if (filename && mode)
             {
                open(filename, mode);
            } else {
                printf("type: 'open' <filename> <mode>\n");
            }
        }
        else if (strncmp(command, "close ", 6) == 0) 
        {
            strtok(command, " ");
            char *filename = strtok(NULL, " "); 
            if (filename != NULL) 
            {
                close(filename); 
            } else {
             printf("type: 'close' <filename>\n");
        }
    }
        else if (strncmp(command, "lsof", 4) == 0) 
        {
            lsof();
        } 
        else if (strncmp(command, "lseek", 5) == 0) 
        {
            char *filename = strtok(command + 6, " ");
            char *offsetStr = strtok(NULL, " ");

            if (filename == NULL || offsetStr == NULL) 
            {
                printf("Usage: lseek <filename> <newOffset>\n");
                return;
            }
            else
            {
                uint32_t newOffset = (uint32_t)strtoul(offsetStr, NULL, 10);
                lseek(filename, newOffset);
            }
        } 
        else if (strncmp(command, "read", 4) == 0)
        {
            char *filename = strtok(command + 5, " ");
            char *sizeStr = strtok(NULL, " ");
            char *endptr;

            if (filename == NULL || sizeStr == NULL) 
            {
                printf("Usage: read <filename> <size>\n");
                return;
            }
            uint32_t size = (uint32_t)strtoul(sizeStr, &endptr, 10);
            if (*endptr != '\0') 
            {
                printf("Error: Invalid size value.\n");
                return;
            }

            read(filename, size);
        }
        else if (strncmp(command, "write", 5) == 0)
        {
            char *filename = strtok(command + 6, " ");
            char *string = strtok(NULL, "");

            if (filename == NULL || string == NULL)
            {
                printf("Usage: write <filename> <string>\n");
                return;
            }
            else
            {
                write(filename, string, "w");
            
            }
        }else if (strncmp(command, "rm ", 3) == 0) {
            char *filename = strtok(command + 3, " ");
            if (filename != NULL) {
                rm(filename);
            } else {
                printf("type: rm <filename>\n");
            }
        }
         else if (strncmp(command, "rmdir ", 6) == 0) {
            char *dirname = command + 6;
            if (dirname != NULL && strlen(dirname) > 0) {
                rmdir(dirname);
            } else {
                printf("type: rmdir <dirname>\n");
            }
        }

        else 
        {
            printf("Unknown command: %s\n", command);
        }
    }
}
