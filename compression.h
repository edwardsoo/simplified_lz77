#ifndef SIMPLIFIED_LZ77_H
#define SIMPLIFIED_LZ77_H

#define PTR_SIZE 0x1000
void compress_file (FILE *in, FILE *out);
void decompress_file (FILE *in, FILE *out);

#endif
