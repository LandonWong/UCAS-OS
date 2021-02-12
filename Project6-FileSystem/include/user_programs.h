
extern unsigned char _elf___test_test_shell_elf[];
int _length___test_test_shell_elf;
extern unsigned char _elf___test_recv_elf[];
int _length___test_recv_elf;
extern unsigned char _elf___test_test_fs_elf[];
int _length___test_test_fs_elf;
extern unsigned char _elf___test_race_elf[];
int _length___test_race_elf;
extern unsigned char _elf___test_lfs_elf[];
int _length___test_lfs_elf;
extern unsigned char _elf___test_back_elf[];
int _length___test_back_elf;
extern unsigned char _elf___test_ftp_file_elf[];
int _length___test_ftp_file_elf;
extern unsigned char _elf___test_ftp_recv_elf[];
int _length___test_ftp_recv_elf;
extern unsigned char _elf___test_read_elf[];
int _length___test_read_elf;
typedef struct ElfFile {
  char *file_name;
  unsigned char* file_content;
  int* file_length;
} ElfFile;

#define ELF_FILE_NUM 9
extern ElfFile elf_files[9];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);
