#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."
#define OS_SIZE_LOC 0x1fc

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE * img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1] [0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    } 
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    } 
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 0;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;
    /* open the image file */
	img = fopen(IMAGE_FILE,"wb");
	if(img == NULL)
		printf("[ERROR] Failed to open IMAGE file.\n");
    /* for each input file */
      while (nfiles-- > 0) { 
		/* open input file */
		fp = fopen(*files, "rb");
		if(!fp)
			printf("[ERROR] Reading file %s\n",*files);
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);
        /* for each program header */
        printf("File: %s has %d segments.\n", *files, ehdr.e_phnum);
		for (ph = 0; ph < ehdr.e_phnum; ph++) {
			/* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
           }
        fclose(fp);
        files++;
    }
    write_os_size(nbytes, img);
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
	if(!fread(ehdr, sizeof(Elf64_Ehdr), 1, fp))
		printf("[ERROR] Failed to read ehdr.\n");
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
	fseek(fp, (long)(ehdr.e_phoff + ehdr.e_phentsize * ph), SEEK_SET);
	if(!fread(phdr, sizeof(Elf64_Phdr), 1, fp))
		printf("[ERROR] Failed to read phdr header");
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
	size_t file_size = phdr.p_filesz;
	size_t offset = phdr.p_offset;
	size_t mem_size = phdr.p_memsz;
	// calculate the size (x * 512B)
	int size = (mem_size + 511) / 512 * 512;
	char data[size];
	fseek(fp, (long)(offset), SEEK_SET);
	fread(data, file_size, 1, fp);
	fseek(img, (long)(*first * 512), SEEK_SET);
	fwrite(data, size, 1, img);
	// refresh nbytes
	*nbytes += size;
    *first += size / 512;
}
static void write_os_size(int nbytes, FILE * img)
{
    // calculate the kernel size
	int kernel_size = (nbytes >> 9) - 1;
	// calculate the pointer
	fseek(img, (long)(OS_SIZE_LOC), SEEK_SET);
	// save kernel size to memory (little endian), two bytes number
	char data[2] = {kernel_size & 0xff, (kernel_size & 0xff00) >> 8};
	fwrite(data, 1, 2, img);
	printf("Kernel Size: %d\n", kernel_size);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
