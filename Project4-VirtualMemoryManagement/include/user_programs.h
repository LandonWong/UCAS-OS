
extern unsigned char _elf___test_test_shell_elf[];
int _length___test_test_shell_elf;
extern unsigned char _elf___test_rw_elf[];
int _length___test_rw_elf;
extern unsigned char _elf___test_fly_elf[];
int _length___test_fly_elf;
extern unsigned char _elf___test_consensus_elf[];
int _length___test_consensus_elf;
extern unsigned char _elf___test_swap_elf[];
int _length___test_swap_elf;
typedef struct ElfFile {
  char *file_name;
  unsigned char* file_content;
  int* file_length;
} ElfFile;

#define ELF_FILE_NUM 5
extern ElfFile elf_files[5];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);
