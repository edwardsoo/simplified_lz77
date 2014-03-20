#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define where() printf("%u\n", __LINE__)

typedef struct bit_stream {
  FILE *file;

  /* The position of the next bit to read in a byte:
   * If it's a 0 then the next bit is the the MSB of the next byte in stream.
   * Otherwise it's the bit_pos-th MSB of last_byte.
   */
  uint8_t bit_pos;
  uint8_t last_byte;

  /*
   * Size of file in bytes and number of bytes read.
   */
  uint64_t file_size;
  uint64_t read;
} bit_stream_t;


bit_stream_t* bit_stream_new (FILE *file) {
  struct stat file_stat;
  bit_stream_t* stream;

  if (fstat (fileno (file), &file_stat) != 0) {
    return NULL;
  }

  stream = malloc (sizeof (bit_stream_t));
  if (stream) {
    stream->file = file;
    stream->file_size = file_stat.st_size;
    stream->read = 0;
    stream->bit_pos = 0;
    stream->last_byte = 0;
  }
  return stream;
}

/*
 * Read 1,4,8 or 12 bits from stream starting at the next bit position 
 * in the bit stream, value is put in result
 * Return 0 for success and -1 for failure.
 * Value of result is not modified if operation failed.
 */
int read_12bits (bit_stream_t *stream, uint16_t *result) {
  uint16_t value;
  int c;

  if (stream->bit_pos == 0) {
    // Cannot use last_byte; need to read 2 bytes from stream
    if (stream->file_size < stream->read + 2) {
      return -1;
    }
    c = fgetc (stream->file);
    value = (c << 4);
    c = fgetc (stream->file);
    value |= (c >> 4);
    stream->last_byte = c;
    stream->bit_pos = 4;
    stream->read += 2;

  } else if (stream->bit_pos > 4) {
    // last_byte has less than 4 unused bits; need to read 2 bytes from stream
    if (stream->file_size < stream->read + 2) {
      return -1;
    }
    value = (stream->last_byte << (stream->bit_pos + 4));
    c = fgetc (stream->file);
    value |= (c << (stream->bit_pos - 4));
    c = fgetc (stream->file);
    value |= (c >> (12 - stream->bit_pos));
    stream->last_byte = c;
    stream->bit_pos -= 4;
    stream->read += 2;

  } else {
    // last_byte has at least 4 unused bits; need to read 1 byte from stream
    if (stream->file_size < stream->read + 1) {
      return -1;
    }
    value = (stream->last_byte << (stream->bit_pos + 4));
    c = fgetc (stream->file);
    value |= (c >> (4 - stream->bit_pos));
    stream->last_byte = c;
    stream->bit_pos = (stream->bit_pos + 4) % 8;
    stream->read += 1;
  }

  *result = (value & 0x0000FFF);
  return 0;
}

int read_1bit (bit_stream_t *stream, uint8_t *result) {
  uint8_t value;
  int c;

  if (stream->bit_pos) {
    // last_byte has at least 1 unused bit; do not need to read from stream
    value = (stream->last_byte >> (7 - stream->bit_pos));

  } else {
    // Cannot use last_byte; need to read 1 byte from stream
    if (stream->read >= stream->file_size) {
      return -1;
    }
    c = fgetc (stream->file);
    value = (c >> 7);
    stream->last_byte = c;
    stream->read += 1;
  }

  stream->bit_pos = (stream->bit_pos + 1) % 8;
  *result = (value & 0x1);
  return 0;
}

int read_4bits (bit_stream_t *stream, uint8_t *result) {
  uint8_t value;
  int c;

  if (stream->bit_pos == 0) {
    // Cannot use last_byte; need read 1 byte from stream
    if (stream->file_size < stream->read + 1) {
      return -1;
    }
    c = fgetc (stream->file);
    value = (c >> 4);
    stream->last_byte = c;
    stream->read += 1;
    stream->bit_pos = 4;

  } else if (stream->bit_pos > 4) {
    // last_byte has less than 4 unused bits; need to read 1 byte from stream
    if (stream->file_size < stream->read + 1) {
      return -1;
    }
    value = (stream->last_byte << (stream->bit_pos - 4));
    c = fgetc (stream->file);
    value |= (c >> (12 - stream->bit_pos));
    stream->last_byte = c;
    stream->read += 1;
    stream->bit_pos = stream->bit_pos - 4;

  } else {
    // last_byte has at least 4 unused bits; do not need to read from stream
    value = (stream->last_byte >> (4 - stream->bit_pos));
    stream->bit_pos += 4;
  }

  *result = (value & 0xF);
  return 0;
}

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
  print_bytes (in);
  // print_12bits(in);
  // print_4bits(in);
  print_bits(in);

  fclose (in);
  fclose (out);
  return 0;
}
