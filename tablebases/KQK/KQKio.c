#include "KQK.h"

int create_file(char *data, long length, char *filename) {
    FILE *fp;
    size_t bytes_written;
    fp = fopen(filename, "wb");
    bytes_written = fwrite(data, sizeof(char), length, fp);
    fclose(fp);
    return (((long)bytes_written == length) ? 0 : 1);
}