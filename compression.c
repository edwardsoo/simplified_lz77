#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bit_stream.h"
#include "compression.h"
#include "queue.h"
#include "prefix_tree.h"

void compress_file (FILE *in, FILE *out) {
  int c;
  bit_out_stream_t *out_stream;
  queue_t *queue;

  out_stream = bit_out_stream_new (out);
  queue = queue_new (0x1000);

  while ((c = fgetc (in)) != EOF) {
    if (write_1bit (out_stream, 0) != 0) break;
    queue_add (queue, c);
    if (write_8bits (out_stream, c) != 0) break;
  }

  bit_out_stream_destroy (&out_stream);
  queue_destroy (&queue);
  fclose (in);
}

void decompress_file (FILE *in, FILE *out) {
  uint8_t op_bit, byte, length, *copy, i;
  uint16_t pointer;
  bit_in_stream_t *in_stream;
  queue_t *queue;

  in_stream = bit_in_stream_new (in);
  queue = queue_new (0x1000);

  while (read_1bit (in_stream, &op_bit) == 0) {
    if (!op_bit) {
      if (read_8bits (in_stream, &byte) != 0) break;
      queue_add (queue, byte);
      fputc (byte, out);
    } else {
      if (read_12bits (in_stream, &pointer) != 0) break;
      if (read_4bits (in_stream, &length) != 0) break;

      // Check if the queue has buffered enough bytes to copy
      if (length > pointer + 1) {
        for (i = 0; i < length; i++) {
          queue_get (queue, queue->length - 1 - pointer, &byte);
          queue_add (queue, byte);
          fputc (byte, out);
        }
      } else {
        copy = queue_sub_array (queue, queue->length - 1 - pointer, length);
        for (i = 0; i < length; i++) {
          queue_add (queue, copy[i]);
        }
        fwrite (copy, 1, length, out);
        free (copy);
      }
    }
  }

  bit_in_stream_destroy (&in_stream);
  queue_destroy (&queue);
  fclose (out);
}

