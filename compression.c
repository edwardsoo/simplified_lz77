#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bit_stream.h"
#include "compression.h"
#include "queue.h"
#include "prefix_tree.h"

#define PTR_SIZE 0x1000
#define WHERE() printf("%d\n", __LINE__)

void print_key_bytes (uint8_t *key, int len) {
  int i;
  for (i = 0; i < len; i++) {
    printf ("0x%x ", key[i]);
  }
}

void print_key_chars (uint8_t *key, int len) {
  int i;
  for (i = 0; i < len; i++) {
    printf ("%c", key[i]);
  }
}

void print_tree_inorder (prefix_tree_t *tree, int depth) {
  int i;
  if (tree) {
    printf ("%*sprefix = [", depth, "");
    print_key_chars (tree->key, tree->key_len);
    if (tree->has_value) {
      printf("] value = %ld", tree->value);
    } else {
      printf("] value = NULL");
    }
    printf (" depth %d\n", depth);
    for (i = 0; i < 0x100; i++) {
      print_tree_inorder (tree->child[i], depth + 1);
    }
  }
}

int value_less_than (prefix_tree_t *tree, void *arg) {
  uint64_t *value = arg;
  return tree->value < *value;
}

void insert_queue_head_prefixes (
    prefix_tree_t **tree_p, queue_t *queue, uint64_t value
    ) {
  int i;
  uint8_t *key;

  for (i = queue->length; i > 1; i--) {
    key = queue_sub_array (queue, 0, i);
    prefix_tree_insert (tree_p, key, i, value);
    free (key);
  }
  print_tree_inorder (tree);
}

void compress_file (FILE *in, FILE *out) {
  int c, i, matched;
  uint8_t byte, *key;
  uint16_t pointer;
  uint64_t read, compressed, value;
  bit_out_stream_t *out_stream;
  prefix_tree_t *tree;
  queue_t *pointable, *pending;

  // Buffer pointable compressed bytes
  pointable = queue_new (PTR_SIZE);
  // <pointer,len> can be <0,15>
  pending = queue_new (0xF);
  out_stream = bit_out_stream_new (out);
  tree =prefix_tree_new (NULL, 0, 0, 0);
  read = 0;
  compressed = 0;

  while ((c = fgetc (in)) != EOF || pending->length) {

    // printf ("compressed %ld bytes, %d pending bytes\n", compressed, pending->length);
    // print_tree_inorder (tree, 0);

    // Compress a byte if queue pending is full or finished reading from file
    if (pending->length == pending->size || c == EOF) {
      // Find the longest prefix match
      key = queue_sub_array (pending, 0, pending->length);
      matched = prefix_tree_longest_match (tree, key, pending->length, &value);

      if (matched) {
        // printf ("longest match with %d [", pending->length);
        //print_key_chars (key, pending->length);
        //printf ("] found match = [");
        //print_key_chars (key, matched);
        //printf ("] value = %ld\n", value);
      }
      free (key);

      // Insert subarrays starting at this byte into prefix tree
      insert_queue_head_prefixes (&tree, pending, compressed + 1);

      queue_pop (pending, &byte);
      printf ("processing '%c': ", byte);

      // Write compressed data
      if (matched && compressed >= value) {
        assert (matched >= 2 && matched <= 15);
        pointer = compressed - value;
        if (write_1bit (out_stream, 1) != 0) break;
        if (write_12bits (out_stream, pointer) != 0) break;
        if (write_4bits (out_stream, matched) != 0) break;
        printf ("<1,%d,%d>\n", pointer, matched);

        // pipe bytes from queue pending to queue pointable
        for (i = 1; i < matched; i++) {
          queue_add (pointable, byte);
          insert_queue_head_prefixes (&tree, pending, compressed + 1);
          queue_pop (pending, &byte);
          compressed += 1;
        }

      } else {
        if (write_1bit (out_stream, 0) != 0) break;
        if (write_8bits (out_stream, byte) != 0) break;
        printf ("<0,'%c'>\n", byte);
      }
      compressed += 1;

      if (pointable->length == pointable->size) {
        // Remove all subarrays with lengths between 2 and 15 begining with
        // the oldest byte in queue pointable and has a value less than 
        // [compressed - PTR_SIZE] from the prefix tree

        value = compressed - PTR_SIZE;
        // Prefix lengths in [2,15]
        for (i = 2; i < 0x10; i++) {
          key = queue_sub_array (pointable, 0, i);
          prefix_tree_delete (&tree, key, i, value_less_than, &value);
          free (key);
        }
      }
      queue_add (pointable, byte);
    }

    if (c != EOF) {
      // Read a byte from file
      queue_add (pending, c);
      read += 1;
    }
  }

  bit_out_stream_destroy (&out_stream);
  prefix_tree_destroy (&tree);
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

  while (read_1bit (in_stream, &op_bit) == 0) {
    if (!op_bit) {
      if (read_8bits (in_stream, &byte) != 0) break;
      printf ("<0,'%c'>\n", byte);
      queue_add (queue, byte);
      fputc (byte, out);

    } else {
      if (read_12bits (in_stream, &pointer) != 0) break;
      if (read_4bits (in_stream, &length) != 0) break;
      printf ("<1,%d,%d>\n", pointer, length);

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

