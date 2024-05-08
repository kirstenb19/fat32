## Project 3

In this project, we familiarize ourselves with basic file-system design and implementation, while
understanding various aspects of the FAT32 file system, such as cluster-based storage, FAT
tables, sectors, and directory structure.

## Group Number
4

## Group Members
- Stephanie La Rosa: sl21k@fsu.edu
- Yelena Trunima: yt21c@fsu.edu
- Kirsten Blair: kmb22@fsu.edu

## Division of Labor

### Part 1: Mount the Image File
- **Responsibilities**: 
- [X] Mount the image file through command line arguments.
- **Assigned to**: Yelena Trunima
- Completed by: Yelena Trunima

### Part 2: Navigation
- **Responsibilities**:
- [X]  Create "cd [DIRNAME]", and "ls", that will allow the user to navigate the mounted image.
- **Assigned to**: Yelena Trunima
- Completed by: Yelena Trunima

### Part 3: Create
- **Responsibilities**:
- [X] Create commands that will allow the user to create files and directories with "mkdir [DIRNAME]" and "creat [FILENAME]"
- **Assigned to**: Yelena Trunima
- Completed by: Yelena Trunima

### Part 4: Read
- **Responsibilities**:
- [X] Create commands that will read from opened files with a structure that stores which files are opened.
- **Assigned to**: Kirsten Blair & Stephanie La Rosa
- Completed by: Kirsten Blair & Stephanie La Rosa

### Part 5: Update
- **Responsibilities**:
- [X] Implement the functionality to allow the user to write to a file using "write [FILENAME] [STRING]"
- **Assigned to**: Stephanie La Rosa
- Completed by: Stephanie La Rosa

### Part 6: Delete
- **Responsibilities**:
- [X] Implement the functionality that allows the user to delete a file/directory with "rm [FILENAME]" and "rmdir [DIRNAME]"
- **Assigned to**: Kirsten Blair
- Completed by: Kirsten Blair

## File Listing
```
main/
│
├── .gitignore
├── Makefile
├── README.md
├── include/
    ├── fat32.h
    └── shell.h
└── src/
    ├── fat32.c
    ├── shell.c
    └── filesys.c
```
## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.

### Compilation
For a C/C++ example:
```bash
make
```
