#include "cpio.h"
#include "string.h"
#include "type.h"
#include "uart.h"


static unsigned long align_up(unsigned long n, unsigned long align)
{
    return (n + align - 1) & (~(align - 1));
}

/* Parse an ASCII hex string into an integer. */
static unsigned long parse_hex_str(char *s, unsigned int max_len)
{
    unsigned long r = 0;
    unsigned long i;

    for (i = 0; i < max_len; i++) {
        r *= 16;
        if (s[i] >= '0' && s[i] <= '9') {
            r += s[i] - '0';
        }  else if (s[i] >= 'a' && s[i] <= 'f') {
            r += s[i] - 'a' + 10;
        }  else if (s[i] >= 'A' && s[i] <= 'F') {
            r += s[i] - 'A' + 10;
        } else {
            return r;
        }
        continue;
    }
    return r;
}

int cpio_parse_header(struct cpio_header *archive,
        const char **filename, unsigned long *_filesize, void **data,
        struct cpio_header **next)
{
    unsigned long filesize;
    /* Ensure magic header exists. */
    if (strncmp(archive->c_magic, CPIO_HEADER_MAGIC,
                sizeof(archive->c_magic)) != 0)
        return -1;

    /* Get filename and file size. */
    filesize = parse_hex_str(archive->c_filesize, sizeof(archive->c_filesize));
    *filename = ((char *)archive) + sizeof(struct cpio_header);

    /* Ensure filename is not the trailer indicating EOF. */
    if (strncmp(*filename, CPIO_FOOTER_MAGIC, sizeof(CPIO_FOOTER_MAGIC)) == 0)
        return 1;

    /* Find offset to data. */
    unsigned long filename_length = parse_hex_str(archive->c_namesize,
            sizeof(archive->c_namesize));
    *data = (void *)align_up(((unsigned long)archive)
            + sizeof(struct cpio_header) + filename_length, CPIO_ALIGNMENT);
    *next = (struct cpio_header *)align_up(((unsigned long)*data) + filesize, CPIO_ALIGNMENT);
    if(_filesize){
        *_filesize = filesize;
    }
    return 0;
}


int cpio_info(void *archive, struct cpio_info *info) {
    struct cpio_header *header, *next;
    const char *current_filename;
    void *result;
    int error;
    unsigned long size, current_path_sz;

    if (info == NULL) return 1;
    info->file_count = 0;
    info->max_path_sz = 0;

    header = archive;
    while (1) {
        error = cpio_parse_header(header, &current_filename, &size,
                &result, &next);

        if (error == -1) 
            return error;
        else if (error == 1) 
            return 0;
    
        info->file_count++;
        header = next;

        // Check if this is the maximum file path size.
        current_path_sz = strlen(current_filename);
        if (current_path_sz > info->max_path_sz) {
            info->max_path_sz = current_path_sz;    
        }
    }

    return 0;
}


void cpio_ls(void *archive, char buf[100][100], unsigned long buf_len) {
    const char *current_filename;
    struct cpio_header *header, *next;
    void *result;
    int error;
    unsigned long i, size;

    header = archive;
    for (i = 0; i < buf_len; i++) {
        error = cpio_parse_header(header, &current_filename, &size,
                &result, &next);
        // Break on an error or nothing left to read.
        if (error) break;
        strcpy(buf[i],  current_filename);
        header = next;
    }
}