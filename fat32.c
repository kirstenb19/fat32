#include "fat32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

#define FILE_LENGTH 11
#define MODE_LENGTH 3
#define OPEN_FILES 10


FILE *imageFile = NULL;
BootSector bootSector;
uint32_t currentDirectoryCluster = 2;

typedef struct {
    uint32_t cluster;
    char path[260];
} DirectoryStackEntry;

DirectoryStackEntry dirStack[256];
int stackPointer = -1; // starting empty init

char imageName[260] = ""; // empty str init
char currentPath[260] = "/"; // starting at root

int mount_image(const char *imagePath) {
    // opening in binary mode
    imageFile = fopen(imagePath, "r+b");

    // check if opoen successful
    if (!imageFile) {
        printf("Error: Could not open image file %s\n", imagePath);
        return 0;
    }

    // reading the boot sector fields
    fseek(imageFile, 11, SEEK_SET);
    fread(&bootSector.bytesPerSector, sizeof(uint16_t), 1, imageFile);
    fread(&bootSector.sectorsPerCluster, sizeof(uint8_t), 1, imageFile);
    fread(&bootSector.reservedSectorCount, sizeof(uint16_t), 1, imageFile);
    fread(&bootSector.numFATs, sizeof(uint8_t), 1, imageFile);
    fseek(imageFile, 36, SEEK_SET);
    fread(&bootSector.fatSize, sizeof(uint32_t), 1, imageFile);
    fseek(imageFile, 44, SEEK_SET);
    fread(&bootSector.rootCluster, sizeof(uint32_t), 1, imageFile);
    fseek(imageFile, 48, SEEK_SET);
    fread(&bootSector.totalDataClusters, sizeof(uint32_t), 1, imageFile);

    // calculating imageSize by moving to the end of the file
    fseek(imageFile, 0, SEEK_END);
    bootSector.imageSize = ftell(imageFile);
    rewind(imageFile); // rewind to the start

    // init the directory stack to root
    init_directory_stack();

    // if success then copy the image name
    strncpy(imageName, imagePath, sizeof(imageName) - 1);
    imageName[sizeof(imageName) - 1] = '\0'; // null term

    return 1;
}

void unmount_image() {
    if (imageFile) {
        fclose(imageFile);
        imageFile = NULL;
    }
}

void print_boot_sector_info() {
    printf("Bytes Per Sector: %d\n", bootSector.bytesPerSector);
    printf("Sectors Per Cluster: %d\n", bootSector.sectorsPerCluster);
    printf("Root Cluster: %d\n", bootSector.rootCluster);
    printf("Total # of Clusters in Data Region: %d\n", bootSector.totalDataClusters);
    printf("# of Entries in One FAT: %d\n", bootSector.fatEntries);
    printf("Size of Image (in bytes): %llu\n", bootSector.imageSize);
}

BootSector get_boot_sector() {
    return bootSector;
}

//part2
void formatFileName(const uint8_t *name, char *formattedName) {
    int nameLen = 0;
    for (int i = 0; i < 11; i++) {
        if (name[i] != ' ') {
            formattedName[nameLen++] = (i == 8) ? '.' : name[i]; // Insert dot before extension
        }
    }
    formattedName[nameLen] = '\0';
}

void list_directory() {
    // use the current directory cluster to get the sector
    uint32_t currentDirSector = bootSector.reservedSectorCount + 
            (bootSector.numFATs * bootSector.fatSize) + ((currentDirectoryCluster - 2) 
            * bootSector.sectorsPerCluster);
    uint64_t offset = currentDirSector * bootSector.bytesPerSector;

    fseek(imageFile, offset, SEEK_SET); // getting to the start of the sector
    
    DirectoryEntry dirEntry;
    char formattedName[13]; // For formatted filename

    // read the entries and print them
    while (fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile)) {
        if (dirEntry.name[0] == 0x00) { // end of directory entries
            break;
        }
        if (dirEntry.name[0] == 0xE5 || (dirEntry.attr & ATTR_VOLUME_ID)) { // skip volume lbl
            continue;
        }

        formatFileName(dirEntry.name, formattedName); // format the name
        if (dirEntry.attr & ATTR_DIRECTORY) { // checking if directory
            printf("[DIR] %s\n", formattedName);    //TODO CAN REMOVE [DIR] to match specs
            } else {
            printf("%s\n", formattedName);
        }
    }
}

void change_directory(const char *dirname) {
    if (strcmp(dirname, ".") == 0) {
        // nothing to do cause we are already in the current directory
        return;
    }
    
    if (strcmp(dirname, "..") == 0) {
        // going back to the prev TODO doesnt work properly -- goes to the root rn
        pop_directory_stack();
        return;
    }

    // for regular case
    uint32_t newCluster = find_directory_cluster(dirname, currentDirectoryCluster);
    if (newCluster != 0) {
        // saving to the stack before changing
        push_directory_stack(currentDirectoryCluster, currentPath);

        // updating the current directory
        currentDirectoryCluster = newCluster;
        append_to_path(currentPath, dirname); // adding the new directory to the name 
    } else {
        // not found
        printf("Directory '%s' not found.\n", dirname);
    }
}

void append_to_path(char *path, const char *dirname) {
    // handling the case where the path is not the root
    if (strcmp(path, "/") != 0) {
        strcat(path, "/");
    }
    strcat(path, dirname);
    path[sizeof(currentPath) - 1] = '\0'; // null term
}

// to find the cluster of the parent directory of the current directory
uint32_t find_directory_cluster(const char *dirname, uint32_t parentCluster) {
    // reding the entries of the parent directory
    DirectoryEntry dirEntry;
    uint32_t firstSector = get_sector_of_cluster(parentCluster);
    uint32_t sector = firstSector;
    // looping through the sectors
    do {
        fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
        for (int i = 0; i < bootSector.sectorsPerCluster * bootSector.bytesPerSector 
                    / sizeof(DirectoryEntry); ++i) {
            fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);
            if (dirEntry.name[0] == 0x00) {
                // no more entries so break
                return 0;
            }
            if (dirEntry.name[0] == 0xE5 || (dirEntry.attr & ATTR_VOLUME_ID)) {
                // skipping volume and invalid entries
                continue;
            }
            char formattedName[13];
            formatFileName(dirEntry.name, formattedName);
            if (strcmp(formattedName, dirname) == 0 && (dirEntry.attr & ATTR_DIRECTORY)) {
                // found
                return (uint32_t)((uint32_t)dirEntry.fstClusHI << 16 | dirEntry.fstClusLO);
            }
        }
        sector = get_next_cluster(sector);
    } while (sector < 0x0FFFFFF8);
    return 0; //not foudn so return 0
}


//finding sector number 
uint32_t get_sector_of_cluster(uint32_t cluster) {
    return ((cluster - 2) * bootSector.sectorsPerCluster) + (bootSector.reservedSectorCount + 
            (bootSector.numFATs * bootSector.fatSize));
}

void pop_directory_stack() {
    // checking if there is a dir to go back to
    if (stackPointer > 0) {
        stackPointer--; // go back one
        // update the current directory and path
        currentDirectoryCluster = dirStack[stackPointer].cluster;
        strncpy(currentPath, dirStack[stackPointer].path, sizeof(currentPath) - 1);
        currentPath[sizeof(currentPath) - 1] = '\0'; // null term 
    }
}

void push_directory_stack(uint32_t cluster, const char* path) {
    // dont push beyond the limit
    if (stackPointer < 256 - 1) {
        stackPointer++;
        dirStack[stackPointer].cluster = cluster;
        strncpy(dirStack[stackPointer].path, path, sizeof(dirStack[stackPointer].path) - 1);
        dirStack[stackPointer].path[sizeof(dirStack[stackPointer].path) - 1] = '\0'; // null term
    } else {
        fprintf(stderr, "Stack overflow error\n");
    }
}

void init_directory_stack() {
    // init with the root
    stackPointer = 0; // root is the first
    dirStack[stackPointer].cluster = bootSector.rootCluster;
    strncpy(dirStack[stackPointer].path, "/", sizeof(dirStack[0].path) - 1);
    dirStack[stackPointer].path[sizeof(dirStack[0].path) - 1] = '\0'; // null term
}


//part 3

int mkdir(const char *dirname) {
    char dirNameFormatted[11];
    format_dir_name(dirname, dirNameFormatted);

    uint32_t newCluster = find_free_cluster();
    if (newCluster == 0) {
        printf("Failed to find a free cluster for the new directory.\n");
        return 0; // Failure
    }

    // marking the new cluster as the last
    update_fat_entry(newCluster, 0x0FFFFFFF);

    DirectoryEntry newDirEntry = {0};
    memcpy(newDirEntry.name, dirNameFormatted, sizeof(newDirEntry.name));
    newDirEntry.attr = ATTR_DIRECTORY;
    newDirEntry.fstClusHI = (newCluster >> 16) & 0xFFFF;
    newDirEntry.fstClusLO = newCluster & 0xFFFF;

    if (!add_directory_entry(&newDirEntry)) {
        printf("Failed to add the new directory entry.\n");
        return 0;
    }

    if (!init_new_directory(newCluster, currentDirectoryCluster)) {
        printf("Failed to initialize the new directory.\n");
        return 0;
    }

    return 1;
}

uint32_t find_free_cluster() {
    uint32_t cluster = 2; // first 
    for (uint32_t fatSector = bootSector.reservedSectorCount; fatSector < bootSector.numFATs 
                * bootSector.fatSize; fatSector++) {
        seek_to_fat(fatSector); // adjusted to the fat sector
        for (uint32_t i = 0; i < bootSector.bytesPerSector / 4; i++) {
            uint32_t entry;
            fread(&entry, sizeof(entry), 1, imageFile);
            if (entry == 0) {
                return cluster;
            }
            cluster++;
        }
    }
    // no free cluster found
    return 0;
}

void update_fat_entry(uint32_t cluster, uint32_t value) {
    seek_to_fat(cluster);
    fwrite(&value, sizeof(uint32_t), 1, imageFile);
}

void seek_to_fat(uint32_t cluster) {
    uint32_t fatSector = bootSector.reservedSectorCount + (cluster * 4) 
                            / bootSector.bytesPerSector;
    uint32_t offset = (cluster * 4) % bootSector.bytesPerSector;
    fseek(imageFile, fatSector * bootSector.bytesPerSector + offset, SEEK_SET);
}

// initializing the new directory
int init_new_directory(uint32_t newDirCluster, uint32_t parentDirCluster) {
    DirectoryEntry dotEntries[2] = {0};

    // .
    strcpy((char *)dotEntries[0].name, ".          ");
    dotEntries[0].attr = ATTR_DIRECTORY;
    dotEntries[0].fstClusHI = (newDirCluster >> 16) & 0xFFFF;
    dotEntries[0].fstClusLO = newDirCluster & 0xFFFF;

    // .. 
    strcpy((char *)dotEntries[1].name, "..         ");
    dotEntries[1].attr = ATTR_DIRECTORY;
    dotEntries[1].fstClusHI = (parentDirCluster >> 16) & 0xFFFF;
    dotEntries[1].fstClusLO = parentDirCluster & 0xFFFF;

    // writing into the first sector of the new directory
    uint32_t sector = get_sector_of_cluster(newDirCluster);
    fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
    fwrite(dotEntries, sizeof(dotEntries), 1, imageFile);
    fflush(imageFile);

    return 1;
}

// formatting the directory name
void format_dir_name(const char *dirname, char *dirNameFormatted) {
    memset(dirNameFormatted, ' ', 11);
    for (int i = 0; i < strlen(dirname) && i < 11; i++) {
        dirNameFormatted[i] = toupper(dirname[i]);
    }
}

int add_directory_entry(DirectoryEntry *entry) {
    uint32_t cluster = currentDirectoryCluster;
    uint32_t sector;
    DirectoryEntry dirEntry;

    // checking if the entry already exists
    while (cluster < 0x0FFFFFF8) { 
        // looping until the end of the cluster chain
        sector = get_sector_of_cluster(cluster);
        for (int i = 0; i < bootSector.sectorsPerCluster; ++i) {
            // looping through the sectors
            fseek(imageFile, (sector + i) * bootSector.bytesPerSector, SEEK_SET);

            // looping through the entries
            for (int j = 0; j < bootSector.bytesPerSector / sizeof(DirectoryEntry); ++j) {
                fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);

                // check for the end of the directory
                if (dirEntry.name[0] == 0x00) {
                    break;
                }
                // skipping deleted entries
                if (dirEntry.name[0] == 0xE5) {
                    continue;
                }
                // check if the entry already exists
                if (strncmp((char*)dirEntry.name, (char*)entry->name, 11) == 0) {
                    printf("Directory/file called %s already exists\n", entry);
                    return -1;
                }
            }
        }
        // getting the next cluster
        cluster = get_next_cluster(cluster);
    }

    // reset the cluster to the current directory
    cluster = currentDirectoryCluster;

    // search for an empty or deleted entry
    while (cluster < 0x0FFFFFF8) {
        sector = get_sector_of_cluster(cluster);
        for (int i = 0; i < bootSector.sectorsPerCluster; ++i) {
            fseek(imageFile, (sector + i) * bootSector.bytesPerSector, SEEK_SET);

            for (int j = 0; j < bootSector.bytesPerSector / sizeof(DirectoryEntry); ++j) {
                fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);

                // check for empty or deleted entry
                if (dirEntry.name[0] == 0x00 || dirEntry.name[0] == 0xE5) {
                    // got a spot
                    fseek(imageFile, -((long)sizeof(DirectoryEntry)), SEEK_CUR);
                    fwrite(entry, sizeof(DirectoryEntry), 1, imageFile);
                    fflush(imageFile); // writing
                    return 1;
                }
            }
        }
        // gettimg the next cluster
        cluster = get_next_cluster(cluster);
    }
    //failed
    return 0;
}

uint32_t get_next_cluster(uint32_t currentCluster) {
    uint32_t fatOffset = currentCluster * 4; //Each FAT32 entry is 4 bytes
    uint32_t fatSector = bootSector.reservedSectorCount + (fatOffset / bootSector.bytesPerSector);
    uint32_t entOffset = fatOffset % bootSector.bytesPerSector;

    // seek to the FAT entry
    fseek(imageFile, (fatSector * bootSector.bytesPerSector) + entOffset, SEEK_SET);

    // readin the next cluster
    uint32_t nextCluster;
    fread(&nextCluster, sizeof(nextCluster), 1, imageFile);

    // maksing to get the last 28 bits
    nextCluster &= 0x0FFFFFFF;

    return nextCluster;
}

// file creation
int creat(const char *filename) {
    char fileNameFormatted[11];
    format_dir_name(filename, fileNameFormatted); // using the same formatting as for dir

    // checking if file or dir already exists
    if (find_directory_cluster(filename, currentDirectoryCluster) != 0) {
        printf("Error: A file or directory named '%s' already exists.\n", filename);
        return -1; // Failure
    }

    // finding a free cluster
    uint32_t newCluster = find_free_cluster();
    if (newCluster == 0) {
        printf("Error: No free clusters available.\n");
        return -1;
    }

    // making the new cluster the last
    update_fat_entry(newCluster, 0x0FFFFFFF);

    DirectoryEntry newFileEntry = {0};
    // copying the name and setting the attributes
    memcpy(newFileEntry.name, fileNameFormatted, sizeof(newFileEntry.name));
    newFileEntry.attr = ATTR_ARCHIVE; // marking as a regular file
    newFileEntry.fstClusHI = (newCluster >> 16) & 0xFFFF;
    newFileEntry.fstClusLO = newCluster & 0xFFFF;
    newFileEntry.fileSize = 0; // init with 0 size

    //failing
    if (!add_directory_entry(&newFileEntry)) {
        printf("Failed to add the new file entry.\n");
        return -1;
    }

    return 1;
}

//part 4 - read 
//structure for the open files 
typedef struct {
    char filename[11]; // no more than 11  one for null 
    char path[256];
    uint32_t cluster;
    int firstClusterChanged;
    char mode[3]; // Includes '\0'
    uint32_t offset;
} OpenFile;


OpenFile openFiles[10];
int Counter = 0; //track number of currenlty opened files 

void open(const char *filename, const char *mode, const char *path)
 {
    if (strcmp(mode, "-r") != 0 && strcmp(mode, "-rw") != 0 &&
        strcmp(mode, "-w") != 0 && strcmp(mode, "-wr") != 0)
         {
        printf("Error... Invalid mode. Only -r, -w, -rw, -wr.\n");
        
        return;
    }

    //check if opened 
    for (int i = 0; i < Counter; i++) 
    {
        if (strncmp(openFiles[i].filename, filename, 11) == 0) 
        {
            printf("Error... File '%s' is already open.\n", filename);
            
            return;
        }
    }

    //locate file in dir 
    uint32_t sector = get_sector_of_cluster(currentDirectoryCluster);
    DirectoryEntry dirEntry;
    int fileExists = 0;
    
     do {
        fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
        for (int i = 0; i < bootSector.bytesPerSector / sizeof(DirectoryEntry); i++) {
            if (!fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile) || dirEntry.name[0] == 0x00) {
                break;  
            }
            if (dirEntry.name[0] == 0xE5 || (dirEntry.attr & ATTR_VOLUME_ID)) 
            {
                continue;  // skipping volume and invalid entries from other fucntion 
            }
            
            char formattedName[12];
            formatFileName(dirEntry.name, formattedName);
            if (strncmp(formattedName, filename, 11) == 0) 
            {
                if (dirEntry.attr & ATTR_DIRECTORY) 
                {
                    printf("Error... '%s' is a directory, not a file.\n", filename);
                    return;
                }
                fileExists = 1; //file has been found 
                break;
            }
        }

        if (!fileExists)
        {
            sector = get_next_cluster(sector);  //move to next cluser
            printf("Error... file does not exist");
        }
    } while (!fileExists && sector < 0x0FFFFFF8 && dirEntry.name[0] != 0x00);

   

    //file open 

    strncpy(openFiles[Counter].filename, filename, FILE_LENGTH - 1);

    openFiles[Counter].cluster = ((uint32_t)dirEntry.fstClusHI << 16) | dirEntry.fstClusLO;//used in earlier functions 
    
    strncpy(openFiles[Counter].mode, mode + 1, MODE_LENGTH - 1); //skip the dash in the mode 

    openFiles[Counter].mode[2] = '\0'; //ensure it is null terminated 
    openFiles[Counter].offset = 0;//record file for offset 
    
    strncpy(openFiles[Counter].path, path, sizeof(openFiles[Counter].path) - 1);
    openFiles[Counter].path[sizeof(openFiles[Counter].path) - 1] = '\0';


    Counter++;

    printf("File '%s' opened. Mode: '%s'.\n", filename, mode + 1);

    //error file limit reached 
    if (Counter >= OPEN_FILES)
     {
        printf("Error...only 10 files can be opened.\n");
        
        return;
    }
}

// lsof function
void lsof() {
    if (Counter == 0) {
        printf("No files are currently open.\n");
        return;
    }

    printf("INDEX\tNAME\t\tMODE\tOFFSET\tPATH\n");
    printf("---------------------------------------------\n");
    for (int i = 0; i < Counter; i++) {
        char fullPath[260]; 
        if (strcmp(openFiles[i].path, "/") == 0) 
        {
            strcpy(fullPath, "/root"); //full path of file with root directory 
        } 
        else 
        {
            snprintf(fullPath, sizeof(fullPath), "/root%s", openFiles[i].path); 
        }
        printf("%d\t%-11s\t%s\t%u\t%s\n",
            i,
            openFiles[i].filename,
            openFiles[i].mode,
            openFiles[i].offset,
            fullPath);
    }
}

// function to get the file size
uint32_t get_file_size(const char *filename) {
    uint32_t sector = get_sector_of_cluster(currentDirectoryCluster);
    DirectoryEntry dirEntry;
    int fileFound = 0;
    uint32_t fileSize = 0;

    while (1) {
        fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
        for (int i = 0; i < bootSector.bytesPerSector / sizeof(DirectoryEntry); i++) {
            fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);
            if (dirEntry.name[0] == 0x00) {
                break;
            }
            if (dirEntry.name[0] == 0xE5 || (dirEntry.attr & ATTR_VOLUME_ID)) {
                continue;
            }

            char formattedName[12];
            formatFileName(dirEntry.name, formattedName);
            if (strncmp(formattedName, filename, 11) == 0) {
                if (dirEntry.attr & ATTR_DIRECTORY) {
                    printf("Error: '%s' is a directory.\n", filename);
                    return 0;
                }
                fileFound = 1;
                fileSize = dirEntry.fileSize;
                break;
            }
        }

        if (fileFound || dirEntry.name[0] == 0x00) {
            break;
        }

        sector = get_next_cluster(sector);
    }

    if (!fileFound) {
        printf("Error: File '%s' not found.\n", filename);
        return 0;
    }

    return fileSize;
}

// lseek function
void lseek(const char *filename, uint32_t newOffset) {
    int fileIndex = -1;

    // checks if the file is open
    for (int i = 0; i < Counter; i++) {
        if (strncmp(openFiles[i].filename, filename, 11) == 0) {
            fileIndex = i;
            break;
        }
    }

    // error message if file is not open 
    if(fileIndex == -1) {
        printf("Error: File '%s' is not open.\n", filename);
        return;
    }

    // gets the size of the file
    uint32_t fileSize = get_file_size(filename);

    // checks if offset is within file size
    if(newOffset > fileSize) {
        printf("Error: Offset is greater than file size.\n");
        return;
    }

    // sets the new offset
    openFiles[fileIndex].offset = newOffset;
    printf("Offset set to %u for the file '%s'.\n", newOffset, filename);
}

// find open files function
OpenFile * find_open_file_by_name(const char *filename) {
    for (int i = 0; i < Counter; i++) {
        if (strncmp(openFiles[i].filename, filename, 11) == 0) {
            return &openFiles[i];
        }
    }

    printf("Error: File '%s' is not open.\n", filename);
    return NULL;
}

// read data helper function
uint32_t read_data(OpenFile *file, char *buffer, uint32_t size) {
    // read the data
    uint32_t bytesRead = 0;
    uint32_t cluster = file->cluster;

    while(bytesRead < size)
    {
        uint32_t sectorOffset = get_sector_of_cluster(cluster) + (file->offset / bootSector.bytesPerSector);
        uint32_t clusterOffset = file->offset % bootSector.bytesPerSector;
        fseek(imageFile, sectorOffset * bootSector.bytesPerSector + clusterOffset, SEEK_SET);

        // calculating how much data to read
        uint32_t toRead = min(size - bytesRead, bootSector.bytesPerSector - clusterOffset);
        fread(buffer + bytesRead, 1, toRead, imageFile);
        bytesRead += toRead;

        // adjust offset
        file->offset += toRead;

        // get the next cluster
        if(file->offset % (bootSector.sectorsPerCluster * bootSector.bytesPerSector) == 0)
        {
            cluster = get_next_cluster(cluster);
        }
    }
    return bytesRead;
}

// print data read helper function
void print_read(const char *filename, char *bytesRead, const char *buffer) {
    // print data read
    printf("Read %u bytes from file '%s':\n", bytesRead, filename);
    for(uint32_t i = 0; i < bytesRead; i++)
    {
        printf("%c", buffer[i]);
    }
    putchar('\n');
}

// read function
void read(const char *filename, uint32_t size)
{
    // checks if the file is open
    OpenFile *file = find_open_file_by_name(filename);
    if(!file) {
        printf("Error: File '%s' is not open or not in read mode.\n", filename);
    }

    // gets the size of the file
    // check if size is greater than file size
    uint32_t fileSize = get_file_size(filename);
    if(file->offset + size > fileSize)
    {
        size = fileSize - file->offset;
    }

    if(size == 0)
    {
        printf("Error: No data to read or the end of the file was reached.\n");
        return;
    }

    // allocate buffer for reading
    char *buffer = (char *)malloc(size);
    if(!buffer)
    {
        printf("Error: Memory allocation failed.\n");
        return;
    }
    else {
        uint32_t bytesRead = read_data(file, buffer, size);
        print_read(filename, bytesRead, buffer);
        free(buffer);
    }
}

// update directory entry helper function
void update_directory_entry(OpenFile *file, uint32_t newSize)
{
    uint32_t sector = get_sector_of_cluster(currentDirectoryCluster);
    DirectoryEntry dirEntry;

    while(1)
    {
        fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
        for(int i = 0; i < bootSector.bytesPerSector / sizeof(DirectoryEntry); i++)
        {
            fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);
            if(dirEntry.name[0] == 0x00)
            {
                break;
            }
            if(dirEntry.name[0] == 0xE5 || (dirEntry.attr & ATTR_VOLUME_ID))
            {
                continue;
            }

            char formattedName[12];
            formatFileName(dirEntry.name, formattedName);
            if(strncmp(formattedName, file->filename, 11) == 0)
            {
                dirEntry.fileSize = newSize;
                fseek(imageFile, -((long)sizeof(DirectoryEntry)), SEEK_CUR);
                fwrite(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);
                fflush(imageFile);
                return;
            }
        }

        if(dirEntry.name[0] == 0x00)
        {
            break;
        }

        sector = get_next_cluster(sector);
    }
}

// write data helper function
uint32_t write_data(OpenFile *file, const char *string, uint32_t stringSize) {
    // write the data
    uint32_t bytesWritten = 0;
    uint32_t cluster = file->cluster;

    while(bytesWritten < stringSize)
    {
        uint32_t sectorOffset = get_sector_of_cluster(cluster) + (file->offset / bootSector.bytesPerSector);
        uint32_t clusterOffset = file->offset % bootSector.bytesPerSector;
        fseek(imageFile, sectorOffset * bootSector.bytesPerSector + clusterOffset, SEEK_SET);

        // calculating how much data to write
        uint32_t toWrite = min(stringSize - bytesWritten, bootSector.bytesPerSector - clusterOffset);
        fwrite(string + bytesWritten, 1, toWrite, imageFile);
        fflush(imageFile);
        bytesWritten += toWrite;

        // adjust offset
        file->offset += toWrite;

        // get the next cluster
        if(file->offset % (bootSector.sectorsPerCluster * bootSector.bytesPerSector) == 0)
        {
            if(cluster >= 0x0FFFFFF8)
            {
                cluster = cluster = get_next_cluster(cluster);
                if(cluster == 0)
                {
                    printf("Error: No more clusters available.\n");
                    break;
                }
            }
        }
    }
    return bytesWritten;
}
// part 5 - write
void write(const char *filename, const char *string, const char *mode) {
    // checks if the file is open
    OpenFile *file = find_open_file_by_name(filename);
    if(!file) {
        printf("Error: File '%s' is not open or not in write mode.\n", filename);
        return;
    }

    // gets the size of the file
    uint32_t fileSize = get_file_size(filename);

    // write the data
    uint32_t stringSize = strlen(string);
    uint32_t newSize = file->offset + stringSize;
    if(newSize > fileSize)
    {
        update_directory_entry(file, newSize);
    }

    // allocate buffer for writing
    char *buffer = (char *)malloc(stringSize);
    if(!buffer)
    {
        printf("Error: Memory allocation failed.\n");
        return;
    }
    else {
        strncpy(buffer, string, stringSize);
        uint32_t bytesWritten = write_data(file, buffer, stringSize);
        printf("Wrote %u bytes to file '%s'.\n", bytesWritten, filename);
        free(buffer);
    }
}

//close the file 
void close(const char *filename) {
    int fileIndex = -1;
   
    for (int i = 0; i < Counter; i++) {
        if (strncmp(openFiles[i].filename, filename, 11) == 0)
         {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1)
     {
        printf("Error.. File '%s' not open or not in cwd.\n", filename);
        return;
    }

    for (int i = fileIndex; i < Counter - 1; i++)
     {
        openFiles[i] = openFiles[i + 1];
    }
    Counter--;

    printf("File '%s' has been closed.\n", filename);
}



//rm the file 
void rm(const char *filename) {
    for (int i = 0; i < Counter; i++) {
        if (strncmp(openFiles[i].filename, filename, 11) == 0) {
            printf("Error... Cannot close open file '%s'\n", filename);
            return;
        }
    }
    uint32_t sector = get_sector_of_cluster(currentDirectoryCluster);
    DirectoryEntry dirEntry;
    long entryPosition = -1;
    int fileF = 0;
    uint32_t fClust;

    do {
        fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
        for (int i = 0; i < bootSector.bytesPerSector / sizeof(DirectoryEntry); i++)
         {
            entryPosition = ftell(imageFile);

            if (!fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile) || dirEntry.name[0] == 0x00) 
            
            {
                fileF = 0;
                break;
            }
            
            
            if (dirEntry.name[0] == 0xE5) continue; //deleted enteries are not shown 

            char formattedName[12];
            formatFileName(dirEntry.name, formattedName);
            if (strncmp(formattedName, filename, 11) == 0)
            
             {
                if (dirEntry.attr & ATTR_DIRECTORY) {
                    printf("Error... '%s' is a directory.\n", filename);
                    return;
                }


                fileF = 1;
                fClust = ((uint32_t)dirEntry.fstClusHI << 16) | dirEntry.fstClusLO;
                break;
            }
        }

        if (fileF) {
            //break the loop if the file has been found 
            break;
        }

        sector = get_next_cluster(sector);
    } while (sector < 0x0FFFFFF8);

    if (!fileF) {
        printf("Error... File '%s' not found.\n", filename);
        return;
    }

    //cluster freeing
    uint32_t cluster = fClust, nextCluster;
   
   
    while (cluster < 0x0FFFFFF8) {
        nextCluster = get_next_cluster(cluster);

        update_fat_entry(cluster, 0x00000000); //cluster is freed 
       
        cluster = nextCluster; 
    }

    fseek(imageFile, entryPosition, SEEK_SET);

    dirEntry.name[0] = 0xE5; //flag for dir 
    
    fwrite(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);
    
    fflush(imageFile);

    printf("File '%s' deleted.\n", filename); //print out success of deleted file 
}

//rmdir 
void rmdir(const char *dirname)
 {
    uint32_t sector = get_sector_of_cluster(currentDirectoryCluster);
    DirectoryEntry dirEntry;
    long entryPosition = -1;
    int dirExists = 0;
    uint32_t dirCluster;

     do {
        fseek(imageFile, sector * bootSector.bytesPerSector, SEEK_SET);
        for (int i = 0; i < bootSector.bytesPerSector / sizeof(DirectoryEntry); i++)
         {
            entryPosition = ftell(imageFile);

            if (!fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile) || dirEntry.name[0] == 0x00)
             {
                dirExists = 0;
                break;
            }

            char formattedName[12];
            formatFileName(dirEntry.name, formattedName);
            if (strncmp(formattedName, dirname, 11) == 0) {
                if (!(dirEntry.attr & ATTR_DIRECTORY))
                 {
                    printf("Error.... '%s' is not a directory.\n", dirname);
                    return;
                }
                dirExists = 1;  //yes, it is there 
                dirCluster = ((uint32_t)dirEntry.fstClusHI << 16) | dirEntry.fstClusLO;
                break;
            }
        }
        //if the directory isnt found, do this 
        if (!dirExists) 
        {
            sector = get_next_cluster(sector);  // Move to next sector
            if (sector >= 0x0FFFFFF8) {
                printf("Error... Directory '%s' not found.\n", dirname);
                return;
            }
        }
        //if directory exisrs, do this 
        if (dirExists) {
            uint32_t dirSector = get_sector_of_cluster(dirCluster);
            fseek(imageFile, dirSector * bootSector.bytesPerSector, SEEK_SET);
            int count = 0, isEmpty = 1;

            while (fread(&dirEntry, sizeof(DirectoryEntry), 1, imageFile) && count < 2) {
                if (!isEmpty) {
                    printf("Error... Directory '%s' isn't empty.\n", dirname);
                    return;
                }

                if (dirEntry.name[0] != 0xE5 && dirEntry.name[0] != 0x00)
                 {
                    char subName[12];
                    formatFileName(dirEntry.name, subName);
                    if (strcmp(subName, "..") != 0 && strcmp(subName, ".") != 0) {
                        isEmpty = 0;
                        break;
                    }
                }
                count++;
            }

            if (!isEmpty)
             {
                printf("Error... Directory '%s' isn't empty.\n", dirname);
                return;
            }

            uint32_t cluster = dirCluster, nextCluster;
            while (cluster < 0x0FFFFFF8) 
            {
                nextCluster = get_next_cluster(cluster);
                update_fat_entry(cluster, 0x00000000);  //clsuter has been freed 
                cluster = nextCluster;
            }

            fseek(imageFile, entryPosition, SEEK_SET);
            dirEntry.name[0] = 0xE5; 

            fwrite(&dirEntry, sizeof(DirectoryEntry), 1, imageFile);

            fflush(imageFile);

            printf("Directory '%s' removed.\n", dirname);
            return;
        }

    
    } while (!dirExists);
}

