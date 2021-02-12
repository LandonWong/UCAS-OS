#ifndef _FS_H
#define _FS_H

#include <type.h>

#define sbsize (sizeof(superblock_t))
#define FS_SIZE     (1 << 19)
#define FS_START    (1 << 12)
#define FS_MAGIC    0x19491001
#define BLOCK_MAP_START     1
#define BLOCK_MAP_NUM       4
#define INODE_MAP_START     5
#define INODE_MAP_NUM       1
#define INODE_START         6
#define INODE_NUM           64
#define DATA_BLOCK_START    70
#define DATA_BLOCK_NUM      (1 << 20) 

#define MAX_DIR_BLK         10

#define O_RDONLY 1 /* read only open */
#define O_WRONLY 2 /* write only open */
#define O_RDWR   3 /* read/write open */

#define I_INODE  1
#define I_DENTRY 2

#define MAX_NAME_LEN 16
#define MAX_FILE_NUM 16

#define D_CUR  0
#define D_PAR  1
#define D_DIR  2
#define D_FILE 3

void mkfs();
void statfs();
void read_dir();
void mkdir(char *);
void rmdir(char *);
void touch(char *);
uint64_t change_dir(char *);
uint64_t cat(char *);
int open(char *name, int access);
int read(int fd, char *buff, int size);
int write(int fd, char *buff, int size);
void close(int fd);
void links(char *src, char *dst);
void linkh(char *src, char *dst);
uintptr_t load_file(char *name, int length);

typedef struct superblock
{
    uint32_t magic;     // Magic number
    uint32_t fs_size;   // file system size
    uint32_t fs_start;  // file system start sector

    uint32_t block_map_start;
    uint32_t block_map_num;

    uint32_t inode_map_start;
    uint32_t inode_map_num;

    uint32_t inode_start;
    uint32_t inode_num;

    uint32_t datablock_start;
    uint32_t datablock_num;
}superblock_t;

typedef struct inode
{
    uint8_t ino;
    uint8_t mode;
    uint8_t type;
    uint8_t num;
    uint16_t used_sz;
    uint16_t create_time;
    uint16_t modify_time;
    uint16_t direct[MAX_DIR_BLK];
    uint16_t level_1;
    uint16_t level_2;
}inode_t;

typedef struct dentry
{
    char name[MAX_NAME_LEN];
    uint8_t type;
    uint8_t ino;
}dentry_t;

typedef struct rec_dir
{
    uint8_t ino;
    char name[16];
}rec_dir_t;

typedef struct file
{
    uint8_t inode;
    uint8_t access;
    uint8_t addr;
    uint32_t r_pos;
    uint32_t w_pos;
}file_t;

extern file_t openfile[MAX_FILE_NUM];

#endif
