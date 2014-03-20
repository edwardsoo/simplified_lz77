#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "simplified_lz77.h"

#define WHERE() printf("%u\n", __LINE__)

void print_uint4_bits (uint8_t value) {
  int mask = 0x8;
  int i;
  for (i = 0; i < 4; i++) {
    printf("%u", !!(mask & value));
    mask = mask >> 1;
  }
  printf("\n");
}

void print_uint8_bits (uint8_t value) {
  int mask = 0x80;
  int i;
  for (i = 0; i < 8; i++) {
    printf("%u", !!(mask & value));
    mask = mask >> 1;
  }
  printf("\n");
}

void print_uint12_bits (uint16_t value) {
  int mask = 0x800;
  int i;
  for (i = 0; i < 12; i++) {
    printf("%u", !!(mask & value));
    mask = mask >> 1;
  }
  printf("\n");
}

void print_bytes(FILE *in) {
  int c;
  fseek (in, 0, SEEK_SET);
  printf("file in bytes\n");
  while ((c = fgetc (in)) != EOF) {
    print_uint8_bits (c);
  }
  printf("\n");
}

void print_bits(FILE *in) {
  uint8_t value;
  int i;
  fseek (in, 0, SEEK_SET);
  bit_stream_t* stream = bit_stream_new (in);
  for (i = 1; read_1bit (stream, &value) == 0; i++) {
    printf("%u", !!(0x1 & value));
    if (i % 8 == 0) {
      printf("\n");
    }
  }
}

void print_4bits(FILE *in) {
  uint8_t value;
  fseek (in, 0, SEEK_SET);
  bit_stream_t* stream = bit_stream_new (in);
  while (read_4bits (stream, &value) == 0) {
    print_uint4_bits (value);
  }
}

void print_12bits(FILE *in) {
  uint16_t value;
  fseek (in, 0, SEEK_SET);
  bit_stream_t* stream = bit_stream_new (in);
  printf ("bit stream: file_size %lu\n", stream->file_size);
  while (read_12bits (stream, &value) == 0) {
    print_uint12_bits (value);
  }
}

void decompress_file (FILE *in, FILE *out) {
  uint8_t op_bit, byte, *copy, i;
  uint16_t pointer;
  bit_stream_t *in_stream;
  queue_t *queue;

  in_stream = bit_stream_new (in);
  queue = queue_new (0x1000);
  while (read_1bit (in_stream, &op_bit) == 0) {
    if (!op_bit) {
      if (read_8bits (in_stream, &byte) != 0) {
        break;
      }
      queue_add (queue, byte);
      fputc (byte, out);
    } else {
      if (read_12bits (in_stream, &pointer) != 0) {
        break;
      }
      if (read_4bits (in_stream, &byte) != 0) {
        break;
      }
      copy = queue_sub_array (queue, queue->length - 1 - pointer, byte);
      for (i = 0; i < byte; i++) {
        queue_add (queue, copy[i]);
      }
      fwrite (copy, 1, byte, out);
      free (copy);
    }
  }
  bit_stream_destroy (&in_stream);
  queue_destroy (&queue);
  fclose (out);
}

int main (int argc, char* argv[]) {
  int c;
  int compress;
  char* input_filename;
  FILE *in, *out;

  compress = -1;
  opterr = 0;
  while ((c = getopt (argc, argv, "c:d:")) != -1) {
    switch (c) {
      case 'c':
        input_filename = optarg;
        compress = 1;
        break;
      case 'd':
        input_filename = optarg;
        compress = 0;
        break;
      default:
        printf ("Usage:\n%s -d FILE OUTPUT to decompress FILE\n"
            "%s -c FILE OUTPUT to compress FILE\n", argv[0], argv[0]);
        return 1;
    }
  }

  if (compress == -1 || optind >= argc) {
    printf ("Usage:\n%s -d FILE OUTPUT to decompress FILE\n"
        "%s -c FILE OUTPUT to compress FILE\n", argv[0], argv[0]);
    return 1;
  }

  in = fopen (input_filename, "rb");
  if (!in) {
    printf ("Failed to open file %s\n", input_filename);
    perror ("fopen");
    return 1;
  }

  out = fopen (argv[optind], "wb");
  if (!out) {
    printf("Failed to open file %s\n", argv[optind]);
    perror ("fopen");
    return 1;
  }

  if (!compress) {
    decompress_file (in, out);
  }

  return 0;
}
