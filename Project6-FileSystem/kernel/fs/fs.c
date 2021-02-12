#include <os/list.h>
#include <os/fs.h>
#include <sbi.h>
#include <pgtable.h>
#include <os/stdio.h>
#include <os/mm.h>
#include <os/time.h>
#include <screen.h>

uint8_t current_ino = 0;
uint8_t global_tmp[2048];
uint32_t global_block[1024];
uint32_t global_block_1[1024];
uint8_t program[80000];
file_t openfile[MAX_FILE_NUM];

uintptr_t load_file(char *name, int length)
{
    int fd = open(name, O_RDWR);
    char *buff = program;
    prints("Copying...\n");
    screen_reflush();
    kmemset(program, 0, 80000);
    while (length > 0)
    {
        read(fd, buff, 512);
        buff += 512;
        length -= 512;
    }
    prints("Finish...\n");
    screen_reflush();
    close(fd);
    return program;
}

int check_fs()
{
   uint8_t tmp[512];
   superblock_t *sb = (superblock_t *)tmp;
   sbi_sd_read(kva2pa(sb), 1, FS_START);
   if (sb->magic == FS_MAGIC)
       return 1;
   return 0;
}

void mkfs()
{
    uint8_t *tmp = (uint8_t *)kmalloc(2048);
    superblock_t *sb = (superblock_t *)tmp;
    prints("> FS: Setting SUPERBLOCK...\n");
    screen_reflush();
    // Check if exists
    /*
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    if (sb->magic == FS_MAGIC)
    {
        prints("> FS: File System already exists.\n");
        statfs();
        return;
    }*/
    // Set SUPERBLOCK
    sb->magic = FS_MAGIC;
    sb->fs_size = FS_SIZE;
    sb->fs_start = FS_START;
    sb->block_map_start = BLOCK_MAP_START;
    sb->block_map_num = BLOCK_MAP_NUM;
    sb->inode_map_start = INODE_MAP_START;
    sb->inode_map_num = INODE_MAP_NUM;
    sb->inode_start = INODE_START;
    sb->inode_num = INODE_NUM;
    sb->datablock_start = DATA_BLOCK_START;
    sb->datablock_num = DATA_BLOCK_NUM;
    prints("> FS: Start initialize filesystem.\n");
    screen_reflush();
    // Write to SD card
    sbi_sd_write(kva2pa(sb), 1, FS_START);
    kmemset(sb, 0, sizeof(sb));
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // Check and print info 
    prints("      magic: 0x%x (KFS).\n", sb->magic);
    prints("      start of sectors: %d (total: %d)\n",
            sb->fs_size, sb->fs_start);
    prints("      block map offset: %d (total: %d)\n",
            sb->block_map_start, sb->block_map_num);
    prints("      inode map offset: %d (total: %d)\n",
            sb->inode_map_start, sb->inode_map_num);
    prints("      inode offset: %d (total: %d)\n",
            sb->inode_start, sb->inode_num);
    prints("      block offset: %d (total: %d)\n",
            sb->datablock_start, sb->datablock_num);
    screen_reflush();
    // Setting inode-map && block-map
    kmemset(tmp, 0, 2048);
    prints("> FS: Setting inode-map.\n");
    sbi_sd_write(kva2pa(tmp)
            , INODE_MAP_NUM, FS_START + INODE_MAP_START);
    prints("> FS: Setting block-map.\n");
    sbi_sd_write(kva2pa(tmp)
            , BLOCK_MAP_NUM, FS_START + BLOCK_MAP_START);
    // Setting directory `/'
    //  setting inode
    inode_t *inode = (inode_t *)tmp;
    inode->ino = 0;
    inode->mode = O_RDWR; 
    inode->num = 0;
    inode->used_sz = 1;
    inode->create_time = get_timer();
    inode->modify_time = get_timer();
    inode->direct[0] = 0;
    sbi_sd_write(kva2pa(inode), 1, FS_START + INODE_START);
    //  setting inode-map
    kmemset(tmp, 0, 512);
    tmp[0] = 1;
    sbi_sd_write(kva2pa(tmp)
            , INODE_MAP_NUM, FS_START + INODE_MAP_START);
    //  setting data_block_map
    sbi_sd_write(kva2pa(tmp)
            , BLOCK_MAP_NUM, FS_START + BLOCK_MAP_START);
    //  setting dentry
    kmemset(tmp, 0, 512);
    dentry_t *de = (dentry_t *)tmp;
    kmemcpy(de->name, ".", 1);
    de->type = D_CUR;
    de->ino = 0;
    prints("> FS: Initialize filesystem finished.\n");
    sbi_sd_write(kva2pa(tmp)
            , 1, FS_START + DATA_BLOCK_START);
    screen_reflush();
}

void statfs()
{
    uint8_t *tmp = (uint8_t *)kmalloc(2048);
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    prints("> FS basic infomation: \n");
    prints("      magic: 0x%x (KFS).\n", sb->magic);
    prints("      start of sectors: %d (total: %d)\n",
            sb->fs_size, sb->fs_start);
    prints("      block map offset: %d (total: %d)\n",
            sb->block_map_start, sb->block_map_num);
    prints("      inode map offset: %d (total: %d)\n",
            sb->inode_map_start, sb->inode_map_num);
    prints("      inode offset: %d (total: %d)\n",
            sb->inode_start, sb->inode_num);
    prints("      block offset: %d (total: %d)\n",
            sb->datablock_start, sb->datablock_num);
    prints("> FS used status: \n");
    uint8_t *tmp1 = (uint8_t *)kmalloc(2048);
    sbi_sd_read(kva2pa(tmp1), sb->inode_map_num, sb->fs_start + sb->inode_map_start);
    int cnt = 0;
    for (int i = 0; i < 512; i++)
    {
        if (tmp1[i]) cnt++;
    }
    prints("  inode used %d/%d\n", cnt, sb->inode_num);
    kmemset(tmp1, 0, 512);
    sbi_sd_read(kva2pa(tmp1), sb->block_map_num, sb->fs_start + sb->block_map_start);
    cnt = 0;
    for (int i = 0; i < 512; i++)
    {
        if (tmp1[i]) cnt++;
    }
    prints("  data block used %d/%d, %dKiB/%dB used\n", cnt, sb->datablock_num, cnt << 2, sb->datablock_num << 2);
    screen_reflush();
}

rec_dir_t get_dir(uint8_t ino, char * name)
{
    rec_dir_t dir;
    int len = kstrlen(name);
    if (len == 0)
    {
        dir.ino = current_ino;
        return dir;
    }
    int pos = 0, cnt = 0, flag = 0;
    for (; pos < len; pos++)
        if (name[pos] == '/') 
        {
            if (flag == 0)
                flag = pos;
            cnt ++;
        }
    if (cnt == 0)
    {
        kmemcpy(dir.name, name, kstrlen(name));
        dir.ino  = ino;
        return dir;
    }
    else
    {
        char cur_name[16];
        char new_name[16];
        kmemset(cur_name, 0 ,16);
        kmemset(new_name, 0 ,16);
        kmemcpy(cur_name, name, flag);
        kmemcpy(new_name, &name[flag + 1], len - flag - 1);
        uint8_t tmp[512];
        uint8_t tmp1[512];
        superblock_t *sb = (superblock_t *)tmp;
        sbi_sd_read(kva2pa(sb), 1, FS_START);
        // find inode
        inode_t *in = (inode_t *)tmp1;
        sbi_sd_read(kva2pa(in), 1, 
                sb->fs_start + sb->inode_start + ino);
        uint32_t start = sb->fs_start + sb->datablock_start;
        for (int i = 0; i < in->used_sz; i++)
        {
            sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
            dentry_t *de = (dentry_t *)tmp;
            if (!kstrcmp(de->name, cur_name) && de->ino != 0xff){
                return get_dir(de->ino, new_name);
            }
        }
        dir.ino  = 0xff;
        return dir;
    }
}

rec_dir_t get_dir_fin(uint8_t ino, char * name)
{
    rec_dir_t dir;
    int len = kstrlen(name);
    if (len == 0)
    {
        dir.ino = ino;
        return dir;
    }
    int pos = 0, cnt = 0, flag = 0;
    for (; pos < len; pos++)
        if (name[pos] == '/') 
        {
            if (flag == 0)
                flag = pos;
            cnt ++;
        }
    if (cnt == 0)
    {
        uint8_t tmp[512];
        uint8_t tmp1[512];
        superblock_t *sb = (superblock_t *)tmp;
        sbi_sd_read(kva2pa(sb), 1, FS_START);
        // find inode
        inode_t *in = (inode_t *)tmp1;
        sbi_sd_read(kva2pa(in), 1, 
                sb->fs_start + sb->inode_start + ino);
        uint32_t start = sb->fs_start + sb->datablock_start;
        for (int i = 0; i < in->used_sz; i++)
        {
            sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
            dentry_t *de = (dentry_t *)tmp;
            if (!kstrcmp(de->name, name) && de->ino != 0xff){
                kmemcpy(dir.name, name, kstrlen(name));
                dir.ino = de->ino;
                return dir;
            }
        }
        dir.ino  = 0xff;
        return dir;
    }
    else
    {
        char cur_name[16];
        char new_name[16];
        kmemset(cur_name, 0 ,16);
        kmemset(new_name, 0 ,16);
        kmemcpy(cur_name, name, flag);
        kmemcpy(new_name, &name[flag + 1], len - flag - 1);
        uint8_t tmp[512];
        uint8_t tmp1[512];
        superblock_t *sb = (superblock_t *)tmp;
        sbi_sd_read(kva2pa(sb), 1, FS_START);
        // find inode
        inode_t *in = (inode_t *)tmp1;
        sbi_sd_read(kva2pa(in), 1, 
                sb->fs_start + sb->inode_start + ino);
        uint32_t start = sb->fs_start + sb->datablock_start;
        for (int i = 0; i < in->used_sz; i++)
        {
            sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
            dentry_t *de = (dentry_t *)tmp;
            if (!kstrcmp(de->name, cur_name) && de->ino != 0xff && de->type != D_FILE){
                return get_dir_fin(de->ino, new_name);
            }
        }
        dir.ino  = 0xff;
        return dir;
    }
}

void read_dir(char *name)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    uint8_t tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // find inode
    rec_dir_t dir;
    if (name[0] != '/')
        dir = get_dir_fin(current_ino, name);
    else
        dir = get_dir_fin(0, &name[1]);
    if (dir.ino == 0xff)
    {
        prints("> Error: No such directory.\n");
        return 0;
    }
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir.ino);
    uint32_t start = sb->fs_start + sb->datablock_start;
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
        dentry_t *de = (dentry_t *)tmp;
        if (de->ino != 0xff)
        prints("%s  ", de->name);
    }
    prints("\n");
}

uint64_t change_dir(char *name)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    uint8_t tmp[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // recursive
    rec_dir_t dir;
    if (name[0] != '/')
        dir = get_dir(current_ino, name);
    else
        dir = get_dir(0, &name[1]);
    if (!kstrcmp(name, "/"))
    {
        current_ino = 0;
        return ;
    }
    if (dir.ino == 0xff)
    {
        prints("> Error: No such directory.\n");
        return 0;
    }
    // find inode
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir.ino);
    uint32_t start = sb->fs_start + sb->datablock_start;
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
        dentry_t *de = (dentry_t *)tmp;
        if (!kstrcmp(de->name, dir.name) && de->ino != 0xff && de->type != D_FILE){
            current_ino = de->ino;
            return 1;
        }
    }
    prints("> Error: No such directory.\n");
    return 0;
}

uint32_t alloc_block()
{
    uint8_t tmp[512];
    uint8_t tmp1[2048];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    sbi_sd_read(kva2pa(tmp1), sb->block_map_num, sb->fs_start + sb->block_map_start);
    int i;
    for (i = 1; i < 2048; i++)
    {
        if (!tmp1[i]) break;
    }
    tmp1[i] = 1;
    sbi_sd_write(kva2pa(tmp1), sb->block_map_num, sb->fs_start + sb->block_map_start);
    return i << 3;
}

void free_block(uint32_t id)
{
    uint8_t tmp[512];
    uint8_t tmp1[2048];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    sbi_sd_read(kva2pa(tmp1), sb->block_map_num, sb->fs_start + sb->block_map_start);
    tmp1[id] = 0;
    sbi_sd_write(kva2pa(tmp1), sb->block_map_num, sb->fs_start + sb->block_map_start);
}

uint32_t alloc_inode()
{
    uint8_t tmp[512];
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    sbi_sd_read(kva2pa(tmp1), sb->inode_map_num, sb->fs_start + sb->inode_map_start);
    int i;
    for (i = 0; i < 512; i++)
    {
        if (!tmp1[i]) break;
    }
    tmp1[i] = 1;
    sbi_sd_write(kva2pa(tmp1), sb->inode_map_num, sb->fs_start + sb->inode_map_start);
    return i;
}

void free_inode(uint8_t ino)
{
    uint8_t tmp[512];
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    sbi_sd_read(kva2pa(tmp1), sb->inode_map_num, sb->fs_start + sb->inode_map_start);
    tmp1[ino] = 0;
    sbi_sd_write(kva2pa(tmp1), sb->inode_map_num, sb->fs_start + sb->inode_map_start);
}

void mkdir(char *name)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    if (!kstrcmp(name, ".") || !kstrcmp(name, ".."))
    {
        prints("> Error: Illegal directory name.\n");
        return;
    }
    uint8_t tmp[512];
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // handle parent inode
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + current_ino);
    in->used_sz++;
    in->num++;
    if (in->used_sz % 8 == 0)
    {
        in->direct[in->used_sz / 8] = alloc_block();
    }
    kmemset(tmp1, 0, 512);
    dentry_t *de = (dentry_t *)tmp1;
    de->type = D_DIR;
    de->ino = alloc_inode();
    kmemcpy(de->name, name, kstrlen(name));
    sbi_sd_write(kva2pa(tmp1), 1, 
            sb->fs_start + sb->datablock_start + in->direct[(in->used_sz - 1) / 8] + (in->used_sz - 1) % 8);
    sbi_sd_write(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + current_ino);
    // handle new inode
    kmemset(global_tmp, 0, 512);
    inode_t *inode = (inode_t *)global_tmp;
    inode->ino = de->ino;
    inode->mode = O_RDWR; 
    inode->num = 0;
    inode->used_sz = 2;
    inode->create_time = get_timer();
    inode->modify_time = get_timer();
    inode->direct[0] = alloc_block();
    sbi_sd_write(kva2pa(inode), 1, sb->fs_start + sb->inode_start + inode->ino);
    kmemset(tmp1, 0, 512);
    de = (dentry_t *)tmp1;
    kmemcpy(de->name, ".", 1);
    de->type = D_CUR;
    de->ino = inode->ino;
    sbi_sd_write(kva2pa(tmp1), 1, 
            sb->fs_start + sb->datablock_start + inode->direct[0]);
    kmemset(tmp1, 0, 512);
    de = (dentry_t *)tmp1;
    kmemcpy(de->name, "..", 2);
    de->type = D_PAR;
    de->ino = current_ino;
    sbi_sd_write(kva2pa(tmp1), 1, 
            sb->fs_start + sb->datablock_start + inode->direct[0] + 1);
    return;
}

void rmdir(char *name)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    if (!kstrcmp(name, ".") || !kstrcmp(name, ".."))
    {
        prints("> Error: Illegal directory name...\n");
        return;
    }
    uint8_t tmp[512];
    uint8_t tmp1[512];
    int tmp_ino = -1;
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + current_ino);
    uint32_t start = sb->fs_start + sb->datablock_start;
    // Search if directory exists
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp1), 1, start + in->direct[i / 8] + i % 8);
        dentry_t *de = (dentry_t *)tmp1;
        if (!kstrcmp(name, de->name) && de->ino != 0xff)
        {    
            tmp_ino = de->ino;
            de->ino = 0xff;
            sbi_sd_write(kva2pa(tmp1), 1, start + in->direct[i / 8] + i % 8);
        }
    }
    if (tmp_ino == -1)
    {
        prints("> Error: No such directory.\n");
        return;
    }
    kmemset(global_tmp, 0, 2048);
    in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + tmp_ino);
    for (int i = 0; i < in->used_sz; i += 4)
    {
        free_block(in->direct[i / 8] >> 3);
    }
    free_inode(tmp_ino);
}

void touch(char *name)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    if (!kstrcmp(name, ".") || !kstrcmp(name, ".."))
    {
        prints("> Error: Illegal directory name.\n");
        return;
    }
    uint8_t tmp[512];
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // handle parent inode
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + current_ino);
    in->used_sz++;
    in->num++;
    if (in->used_sz % 8 == 0)
    {
        in->direct[in->used_sz / 8] = alloc_block();
    }
    kmemset(tmp1, 0, 512);
    dentry_t *de = (dentry_t *)tmp1;
    de->type = D_FILE;
    de->ino = alloc_inode();
    kmemcpy(de->name, name, kstrlen(name));
    sbi_sd_write(kva2pa(tmp1), 1, 
            sb->fs_start + sb->datablock_start + in->direct[(in->used_sz - 1) / 8] + (in->used_sz - 1) % 8);
    sbi_sd_write(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + current_ino);
    // handle new inode
    kmemset(global_tmp, 0, 512);
    inode_t *inode = (inode_t *)global_tmp;
    inode->ino = de->ino;
    inode->mode = O_RDWR; 
    inode->num = 0;
    inode->used_sz = 1;
    inode->create_time = get_timer();
    inode->modify_time = get_timer();
    inode->direct[0] = alloc_block();
    inode->level_1 = 0;
    sbi_sd_write(kva2pa(inode), 1, sb->fs_start + sb->inode_start + inode->ino);
    return;
}

uint64_t cat(char *name)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    uint8_t tmp[512];
    uint8_t tmp1[512];
    uint8_t file_ino = 0xff;
    superblock_t *sb = (superblock_t *)tmp1;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // recursive
    rec_dir_t dir = get_dir(current_ino, name);
    if (dir.ino == 0xff)
    {
        prints("> Error: No such file.\n");
        return 0;
    }
    // find inode
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir.ino);
    uint32_t start = sb->fs_start + sb->datablock_start;
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
        dentry_t *de = (dentry_t *)tmp;
        if (!kstrcmp(de->name, dir.name) && de->ino != 0xff && de->type == D_FILE){
            file_ino = de->ino;
        }
    }
    if (file_ino == 0xff)
    {
        prints("> Error: No such file.\n");
        return 0;
    }
    // show file content
    kmemset(global_tmp, 0, 2048);
    in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + file_ino);
    start = sb->fs_start + sb->datablock_start;
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
        prints("%s", tmp);
    }
    return 0;
}

int open(char *name, int access)
{
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    uint8_t tmp[512];
    uint8_t tmp1[512];
    uint8_t file_ino = 0xff;
    superblock_t *sb = (superblock_t *)tmp1;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // recursive
    rec_dir_t dir = get_dir(current_ino, name);
    if (dir.ino == 0xff)
    {
        prints("> Error: No such file.\n");
        screen_reflush();
        return 66;
    }
    // find inode
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir.ino);
    uint32_t start = sb->fs_start + sb->datablock_start;
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
        dentry_t *de = (dentry_t *)tmp;
        if (!kstrcmp(de->name, dir.name) && de->ino != 0xff && de->type == D_FILE){
            file_ino = de->ino;
            break;
        }
    }
    if (file_ino == 0xff)
    {
        prints("> Error: No such file.\n");
        return 0;
    }
    // alloc a file descriptor
    int id;
    for (id = 0; id < MAX_FILE_NUM; id++)
        if (openfile[id].inode == 0)
            break;
    openfile[id].inode = file_ino;
    openfile[id].access = access;
    openfile[id].addr = 0;
    openfile[id].r_pos = 0;
    openfile[id].w_pos = 0;
    return id;
}

int read(int fd, char *buff, int size)
{
    uint8_t inode = openfile[fd].inode;
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp1;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + inode);
    int sz = size;
    uint32_t start = sb->fs_start + sb->datablock_start;
    // handle unalign
    if (openfile[fd].r_pos % 512 != 0){
        // r_pos < 40KB
        if (openfile[fd].r_pos < 40960){
            uint8_t buffer[512];
            sbi_sd_read(kva2pa(buffer), 1,
                start + in->direct[openfile[fd].r_pos / 512 / 8] 
                + (openfile[fd].r_pos / 512) % 8);
            kmemcpy(buff, &buffer[openfile[fd].r_pos % 512]
                , 512 - openfile[fd].r_pos % 512); 
            sz -= 512 - openfile[fd].r_pos % 512;
            buff += 512 - openfile[fd].r_pos % 512;
        }
        // r_pos < 4MB + 40KB
        else if (openfile[fd].r_pos < 4235264)
        {
            sbi_sd_read(kva2pa(global_block), 4, start + in->level_1);
            uint32_t *level_1 = global_block;
            uint8_t buffer[512];
            int pos = openfile[fd].r_pos - 40960;
            sbi_sd_read(kva2pa(buffer), 1,
                    start + level_1[pos / 512 / 8] + (pos / 512) % 8);
            kmemcpy(buff, buffer, 
                    512 - openfile[fd].r_pos % 512);
            buff += 512 - openfile[fd].r_pos % 512;
            sz -= 512 - openfile[fd].r_pos % 512;
        }
        // r_pos > 4MB + 40KB
        else
        {
            sbi_sd_read(kva2pa(global_block_1), 4, start + in->level_2);
            uint32_t *level_2 = global_block_1;
            int pos = openfile[fd].r_pos - 4235264;
            uint8_t buffer[512];
            sbi_sd_read(kva2pa(global_block), 4,
                    start + level_2[pos / 4194304]);
            uint32_t *level_1 = global_block;
            pos %= 4194304;
            sbi_sd_read(kva2pa(buffer), 1,
                    start + level_1[pos / 512 / 8] + (pos / 512) % 8);
            kmemcpy(buff, buffer,
                    512 - openfile[fd].r_pos % 512);
            buff += 512 - openfile[fd].r_pos % 512;
            sz -= 512 - openfile[fd].r_pos % 512;
        }
    }
    int i;
    for (i = (openfile[fd].r_pos + 511) / 512; i < 80 && sz > 0; sz -= 512)
    {
        uint8_t buffer[512];
        sbi_sd_read(kva2pa(buffer), 1,
                start + in->direct[i / 8] + i % 8);
        kmemcpy(buff, buffer, sz > 512 ? 512 : sz);
        buff += 512;
        i++;
    }
    // > 40KiB
    start = sb->fs_start + sb->datablock_start;
    if (i >= 80 && sz > 0)
    {
        sbi_sd_read(kva2pa(global_block), 4, start + in->level_1);
        uint32_t *level_1 = global_block;
        for (i = openfile[fd].r_pos - 40960; i < 1024 * 512 && sz > 0; sz -= 512)
        {
            uint8_t buffer[512];
            sbi_sd_read(kva2pa(buffer), 1,
                    start + level_1[i / 512 / 8] + (i / 512) % 8);
            kmemcpy(buff, buffer, sz > 512 ? 512 : sz);
            buff += 512;
            i += 512;
        }
    }
    // > 4MB + 40KiB
    if (i == 524288)
    {
        sbi_sd_read(kva2pa(global_block), 4, start + in->level_2);
        uint32_t *level_2 = global_block;
        for (int j = (openfile[fd].r_pos - 4235264) / 4194304;
                j < 1024 && sz > 0; ){
            sbi_sd_read(kva2pa(global_block_1), 4, start + level_2[j]);
            uint32_t *level_1 = global_block_1;
            for (int i = ((openfile[fd].r_pos - 4235264) % 4194304 - 40960);
                    i < 1024 * 512 && sz > 0; sz -= 512)
            {
                uint8_t buffer[512];
                sbi_sd_read(kva2pa(buffer), 1,
                        start + level_1[i / 512 / 8] + (i / 512) % 8);
                kmemcpy(buff, buffer, sz > 512 ? 512 : sz);
                buff += 512;
                i += 512;
            }
            j++;
        }
    }
    openfile[fd].r_pos += size;
    return size;
}

int write(int fd, char *buff, int size)
{
    uint8_t inode = openfile[fd].inode;
    if(!check_fs())
    {
        prints("> Error: No file system.\n");
        return;
    }
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp1;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + inode);
    int sz = size;
    uint32_t start = sb->fs_start + sb->datablock_start;
    // handle unalign
    if (openfile[fd].w_pos % 512 != 0){
        // w_pos < 40KB
        if (openfile[fd].r_pos < 40960){
            uint8_t buffer[512];
            sbi_sd_read(kva2pa(buffer), 1,
                start + in->direct[openfile[fd].w_pos / 512 / 8] 
                + (openfile[fd].w_pos / 512) % 8);
            kmemcpy(&buffer[openfile[fd].w_pos % 512], buff
                , 512 - openfile[fd].w_pos % 512); 
            sbi_sd_write(kva2pa(buffer), 1,
                start + in->direct[openfile[fd].w_pos / 512 / 8] 
                + (openfile[fd].w_pos / 512) % 8);
            sz -= 512 - openfile[fd].w_pos % 512;
            buff += 512 - openfile[fd].w_pos % 512;
        }
        // w_pos > 4MB + 40KB
        else if (openfile[fd].w_pos < 4235264)
        {
            sbi_sd_read(kva2pa(global_block), 4, start + in->level_1);
            uint32_t *level_1 = global_block;
            uint8_t buffer[512];
            int pos = openfile[fd].w_pos - 40960;
            sbi_sd_read(kva2pa(buffer), 1,
                    start + level_1[pos / 512 / 8] + (pos / 512) % 8);
            kmemcpy(&buffer[openfile[fd].w_pos % 512], buff
                    , 512 - openfile[fd].w_pos % 512);
            sbi_sd_write(kva2pa(buffer), 1,
                    start + level_1[pos / 512 / 8] + (pos / 512) % 8);
            kmemcpy(buff, buffer, 
                    512 - openfile[fd].w_pos % 512);
            buff += 512 - openfile[fd].w_pos % 512;
            sz -= 512 - openfile[fd].w_pos % 512;
        }
        // w_pos > 4MB + 40KB
        else
        {
            sbi_sd_read(kva2pa(global_block_1), 4, start + in->level_2);
            uint32_t *level_2 = global_block_1;
            int pos = openfile[fd].r_pos - 4235264;
            uint8_t buffer[512];
            sbi_sd_read(kva2pa(global_block), 4,
                    start + level_2[pos / 4194304]);
            uint32_t *level_1 = global_block;
            pos %= 4194304;
            sbi_sd_read(kva2pa(buffer), 1,
                    start + level_1[pos / 512 / 8] + (pos / 512) % 8);
            kmemcpy(&buffer[openfile[fd].w_pos % 512], buff
                    , 512 - openfile[fd].w_pos % 512);
            sbi_sd_write(kva2pa(buffer), 1,
                    start + level_1[pos / 512 / 8] + (pos / 512) % 8);
            buff += 512 - openfile[fd].r_pos % 512;
            sz -= 512 - openfile[fd].r_pos % 512;
        }
    }
    int i;
    for (i = (openfile[fd].w_pos + 511) / 512;
         i < 80 && sz > 0;
         sz -= 512)
    {
        uint8_t buffer[512];
        if(in->direct[i / 8] == 0)
        {
            in->direct[i / 8] = alloc_block();
            sbi_sd_write(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
            sbi_sd_read(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
        }
        kmemcpy(buffer, buff, sz > 512 ? 512 : sz);
        sbi_sd_write(kva2pa(buffer), 1,
                start + in->direct[i / 8] + i % 8);
        buff += 512;
        i++;
    }
    // > 40KiB
    start = sb->fs_start + sb->datablock_start;
    if (i >= 80 && sz > 0)
    {
        if (in->level_1 == 0)
        {
            in->level_1 = alloc_block();
            sbi_sd_write(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
            sbi_sd_read(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
            memset(global_block, 0, 4096);
            sbi_sd_write(kva2pa(global_block), 4, start + in->level_1);
        }
        uint32_t *level_1 = global_block;
        sbi_sd_read(kva2pa(global_block), 4, start + in->level_1);
        for (i = openfile[fd].w_pos - 40960; i < 1024 * 512 && sz > 0; sz -= 512)
        {
            uint8_t buffer[512];
            if(level_1[i / 512 / 8] == 0)
            {
                level_1[i / 512 / 8] = alloc_block();
                sbi_sd_write(kva2pa(level_1), 4, start + in->level_1);
                sbi_sd_read(kva2pa(level_1), 4, start + in->level_1);
            }
            kmemcpy(buffer, buff, sz > 512 ? 512 : sz);
            sbi_sd_write(kva2pa(buffer), 1,
                    start + level_1[i / 512 / 8] + (i / 512) % 8);
            buff += 512;
            i+=512;
        }
    }
    // > 40MB
    if (i == 524288)
    {
        if (in->level_2 == 0)
        {
            in->level_2 = alloc_block();
            sbi_sd_write(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
            sbi_sd_read(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
            memset(global_block, 0, 4096);
            sbi_sd_write(kva2pa(global_block), 4, start + in->level_2);
        }
        sbi_sd_read(kva2pa(global_block), 4, start + in->level_2);
        uint32_t *level_2 = global_block;
        for (int j = (openfile[fd].w_pos - 4235264) / 4194304;
                j < 1024 && sz > 0; ){
            if (level_2[j] == 0)
            {
                level_2[j] = alloc_block();
                sbi_sd_write(kva2pa(global_block), 4, start + in->level_2);
                sbi_sd_read(kva2pa(global_block), 4, start + in->level_2);
                memset(global_block_1, 0, 4096);
                sbi_sd_write(kva2pa(global_block_1), 4, start + level_2[j]);
            }
            sbi_sd_read(kva2pa(global_block_1), 4, start + level_2[j]);
            uint32_t *level_1 = global_block_1;
            for (int i = ((openfile[fd].w_pos - 4235264) % 4194304 - 40960);
                    i < 1024 * 512 && sz > 0; sz -= 512)
            {
                uint8_t buffer[512];
                if (level_1[i / 512 / 8] == 0)
                {
                    level_1[i / 512 / 8] = alloc_block();
                    sbi_sd_write(kva2pa(global_block_1), 4, start + level_2[j]);
                    sbi_sd_read(kva2pa(global_block_1), 4, start + level_2[j]);
                }
                sbi_sd_read(kva2pa(buffer), 1,
                        start + level_1[i / 512 / 8] + (i / 512) % 8);
                kmemcpy(buffer, buff, sz > 512 ? 512 : sz);
                sbi_sd_write(kva2pa(buffer), 1,
                        start + level_1[i / 512 / 8] + (i / 512) % 8);
                buff += 512;
                i += 512;
            }
            j++;
        }
    }
    openfile[fd].w_pos += size;
    in->used_sz = (openfile[fd].w_pos + 511) / 512;
    sbi_sd_write(kva2pa(in), 1, sb->fs_start + sb->inode_start + inode);
    return size;
}

void close(int fd)
{
    openfile[fd].inode = 0;
    openfile[fd].r_pos = 0;
    openfile[fd].w_pos = 0;
    openfile[fd].access = 0;
}
void links(char *src, char *dst)
{
    rec_dir_t dir_src, dir_dst;
    int spash = kstrlen(src);
    while(spash--)
    {
        if (src[spash] == '/')
            break;
    }
    spash ++;
    if (src[0] != '/')
        dir_src = get_dir(current_ino, src);
    else
        dir_src = get_dir(0, &src[1]);
    if (!kstrcmp(src,"/"))
        dir_src.ino = 0;
        
    if (dir_src.ino == 0xff)
    {
        prints("> No such file or directory.\n");
        return;
    }
    if (dst[0] != '/')
        dir_dst = get_dir_fin(current_ino, dst);
    else
        dir_dst = get_dir_fin(0, &dst[1]);
    if (!kstrcmp(src,"/"))
        dir_dst.ino = 0;
    // Link
    uint8_t tmp[512];
    uint8_t tmp1[512];
    superblock_t *sb = (superblock_t *)tmp1;
    sbi_sd_read(kva2pa(sb), 1, FS_START);
    // find inode
    kmemset(global_tmp, 0, 2048);
    inode_t *in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir_src.ino);
    uint32_t start = sb->fs_start + sb->datablock_start;
    int find = 0;
    for (int i = 0; i < in->used_sz; i++)
    {
        sbi_sd_read(kva2pa(tmp), 1, start + in->direct[i / 8] + i % 8);
        dentry_t *de = (dentry_t *)tmp;
        if (!kstrcmp(&src[spash], de->name) && de->ino != 0xff)
        {
            find = 1;
            break;
        }
    }
    if (find == 0)
    {
        prints("> No such file or dir.\n");
        return;
    }
    kmemset(global_tmp, 0, 2048);
    in = (inode_t *)global_tmp;
    sbi_sd_read(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir_dst.ino);
    in->used_sz++;
    in->num++;
    if (in->used_sz % 8 == 0)
    {
        in->direct[in->used_sz / 8] = alloc_block();
    }
    sbi_sd_write(kva2pa(tmp), 1, 
            sb->fs_start + sb->datablock_start + 
            in->direct[(in->used_sz - 1) / 8] + (in->used_sz - 1) % 8);
    sbi_sd_write(kva2pa(in), 1, 
            sb->fs_start + sb->inode_start + dir_dst.ino);
}

