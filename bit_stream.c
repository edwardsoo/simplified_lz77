#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "simplified_lz77.h"

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
    fseek (file, 0, SEEK_SET);
  }
  return stream;
}

void bit_stream_destroy (bit_stream_t **stream_ptr) {
  bit_stream_t *stream = *stream_ptr;
  fclose (stream->file);
  free (stream);
  *stream_ptr = NULL;
}

/*
 * Read 1,4,8 or 12 bits from stream starting at the next bit position
 * in the bit stream, value is put in result
 * Return 0 for success and -1 for failure.
 * Value of result is not modified if operation failed.
 */
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

int read_8bits (bit_stream_t *stream, uint8_t *result) {
  uint8_t value;
  int c;

  if (stream->file_size < stream->read + 1) {
    return -1;
  }
  value = (stream->last_byte << stream->bit_pos);
  c = fgetc (stream->file);
  value |= (c >> (8 - stream->bit_pos));
  stream->last_byte = c;
  stream->read += 1;

  *result = (value & 0xFF);
  return 0;
}

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
