#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bit_stream.h"
#include "compression.h"
#include "hash.h"
#include "queue.h"

int value_leq (uint64_t value, uint64_t arg) {
  return value <= value;
}

void insert_queue_head_prefixes (
    hash_t *hash, uint8_t *key, int key_len, uint64_t value
    ) {
  int i;
  for (i = 2; i <= key_len; i++) {
    hash_insert (hash, key, i, value);
  }
}

void delete_queue_head_prefixes (
    hash_t *hash, uint8_t *key, int key_len, uint64_t value
    ) {
  int i;
  for (i = key_len; i >= 2; i--) {
    hash_delete (hash, key, i, value_leq, value);
  }
}

int find_longest_prefix_match (
    hash_t *hash, uint8_t *key, int key_len, uint64_t *value
    ) {
  int i;
  for (i = key_len; i > 1; i--) {
    if (hash_lookup (hash, key, i, value) == 0) return i;
  }
  return 0;
}

void compress_file (FILE *in, FILE *out) {
  int c, matched, skip;
  uint8_t byte, *key;
  uint16_t pointer;
  uint64_t compressed, value;
  struct stat file_stat;
  bit_out_stream_t *out_stream;
  hash_t *hash;
  queue_t *pointable, *pending;

  if (fstat (fileno (in), &file_stat) != 0) {
    perror ("fstat");
    fclose (in);
    fclose (out);
    return;
  }

  // Buffer pointable compressed bytes
  pointable = queue_new (PTR_SIZE);
  // <pointer,len> can be <0,15>
  pending = queue_new (0xF);
  out_stream = bit_out_stream_new (out);
  hash = hash_new (PTR_SIZE * 14);
  compressed = 0;

  printf ("Compressing...\n");
  while ((c = fgetc (in)) != EOF || pending->length) {
    if (compressed % 0x100 == 0) {
      printf ("%ld%%\r", compressed * 100 / file_stat.st_size);
    }

    // Compress a byte if queue pending is full or finished reading from file
    if (pending->length == pending->size || c == EOF) {
      // Find the longest prefix match
      key = queue_sub_array (pending, 0, pending->length);
      matched = find_longest_prefix_match (hash, key, pending->length, &value);

      // Insert subarrays starting at this byte into hash table
      insert_queue_head_prefixes (hash, key, pending->length, compressed + 1);
      free (key);

      queue_pop (pending, &byte);
      // printf ("processing '%c': ", byte);

      if (skip > 0) {
        // This byte is already compressed
        skip -= 1;

      } else {
        // Write compressed data
        if (matched && compressed >= value) {
          pointer = compressed - value;
          if (write_1bit (out_stream, 1) != 0) break;
          if (write_12bits (out_stream, pointer) != 0) break;
          if (write_4bits (out_stream, matched) != 0) break;
          // printf ("<1,%d,%d>\n", pointer, matched);

          skip = matched - 1;

        } else {
          if (write_1bit (out_stream, 0) != 0) break;
          if (write_8bits (out_stream, byte) != 0) break;
          // printf ("<0,'%c'>\n", byte);
        }
      }
      compressed += 1;

      if (pointable->length == pointable->size) {
        // Remove all subarrays with lengths between 2 and 15 begining with
        // the oldest byte in queue pointable and has a value less than 
        // [compressed - PTR_SIZE] from the hash table
        key = queue_sub_array (pointable, 0, 0xF);
        delete_queue_head_prefixes (hash, key, 0xF, compressed - PTR_SIZE);
        free (key);
      }
      queue_add (pointable, byte);
    }

    if (c != EOF) {
      // Read a byte from file
      queue_add (pending, c);
    }
  }
  printf("Done\n");

  bit_out_stream_destroy (&out_stream);
  hash_destroy (&hash);
  queue_destroy (&pointable);
  queue_destroy (&pending);
  fclose (in);
}

void decompress_file (FILE *in, FILE *out) {
  uint8_t op_bit, byte, length, *copy, i;
  uint16_t pointer;
  bit_in_stream_t *in_stream;
  queue_t *queue;

  in_stream = bit_in_stream_new (in);
  queue = queue_new (PTR_SIZE);

  printf ("Decompressing...\n");
  while (read_1bit (in_stream, &op_bit) == 0) {
    if (in_stream->read % 0x100 == 0) {
      printf ("%ld%%\r", in_stream->read * 100 / in_stream->file_size);
    }
    if (!op_bit) {
      if (read_8bits (in_stream, &byte) != 0) break;
      // printf ("<0,'%c'>\n", byte);
      queue_add (queue, byte);
      fputc (byte, out);

    } else {
      if (read_12bits (in_stream, &pointer) != 0) break;
      if (read_4bits (in_stream, &length) != 0) break;
      // printf ("<1,%d,%d>\n", pointer, length);

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
  printf("Done\n");

  bit_in_stream_destroy (&in_stream);
  queue_destroy (&queue);
  fclose (out);
}

