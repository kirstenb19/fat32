#ifndef FAT32_H
#define FAT32_H
#define SECTOR_SIZE 512

#include <stdint.h>

// defining the structure
typedef struct {
    uint16_t bytesPerSector;       // size of sector in bytes
    uint8_t sectorsPerCluster;     // num of sectors per cluster which define the size
    uint32_t rootCluster;          // starting cluster of the root dir
    uint32_t totalDataClusters;    // total num of data clusters
    uint32_t fatEntries;           // num of entries
    uint64_t imageSize;            // size of the image
    uint16_t reservedSectorCount;  // num of reserved sectors
    uint8_t numFATs;               // num of FATs
    uint32_t fatSize;              // size in sectors
} BootSector;

typedef struct {
    uint8_t name[11];
    uint8_t attr; // file attr (ex read/hidden/dir/etc)
    uint8_t ntRes; // reserved for use by win NT
    uint8_t crtTimeTenth; // ms stamp at file creation
    uint16_t crtTime; // stamp at file creation
    uint16_t crtDate; // date file
    uint16_t lstAccDate; // Last access date
    uint16_t fstClusHI; // first cluster high word
    uint16_t wrtTime; // time of last write
    uint16_t wrtDate; // date of last write
    uint16_t fstClusLO; // first cluster high word forms the complete cluster number 
                        //where the file's data is stored.
    uint32_t fileSize; // size in bytes
} __attribute__((packed)) DirectoryEntry;

//https://stackoverflow.com/questions/10531203/c-sharp-how-can-i-extract-a-fat-disk-image
//https://www.pjrc.com/tech/8051/ide/fat32.html

extern char imageName[260]; // store the image name
extern char currentPath[260]; // store current path within the image

//part1
int mount_image(const char *imagePath);
void unmount_image();
void print_boot_sector_info();
BootSector get_boot_sector();
//part2
void change_directory(const char *dirname);
void formatFileName(const uint8_t *name, char *formattedName);
void list_directory();
uint32_t find_directory_cluster(const char *dirname, uint32_t parentCluster);
uint32_t get_parent_directory_cluster(DirectoryEntry dirEntry);
uint32_t get_sector_of_cluster(uint32_t cluster);
uint32_t find_next_cluster(uint32_t currentCluster);
void read_cluster(uint32_t clusterNumber, uint8_t* buffer);
int compare_dir_name(const uint8_t* entryName, const char* dirName);
//part3
//create directory
int add_directory_entry(DirectoryEntry *entry);
int init_new_directory(uint32_t newDirCluster, uint32_t parentDirCluster);
int mkdir(const char *dirname);
uint32_t find_free_cluster();
uint32_t get_next_cluster(uint32_t currentCluster);
void seek_to_fat(uint32_t cluster);
//create file
int creat(const char *filename);
//close file
void close(const char *filename);

// attributes for the directory
#define ATTR_DIRECTORY 0x10
#define ATTR_VOLUME_ID 0x08
#define ATTR_LONG_NAME (0x0F)
#define ATTR_ARCHIVE   0x20

#endif
