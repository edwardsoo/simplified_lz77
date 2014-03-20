#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "simplified_lz77.h"

queue_t* queue_new (int size) {
  queue_t *queue;
  queue = malloc (sizeof (queue_t));
  if (queue) {
    queue->array = malloc (sizeof (uint8_t) * size);
    if (!queue->array) {
      free (queue);
      return NULL;
    }
    queue->size = size;
    queue->length = 0;
    queue->head = 0;
  }
  return queue;
}

void queue_destroy (queue_t **queue_ptr) {
  queue_t *queue = *queue_ptr;
  free (queue->array);
  free (queue);
  *queue_ptr = NULL;
}

/*
 * Returns a freshly allocated array containing length-many elements
 * copied from queue, starting at head of queue + offset.
 * Returns NULL if sub_array is out of bound
 */
uint8_t* queue_sub_array (queue_t *queue, int offset, int length) {
  uint8_t *sub_array;
  int copy_head, first_copy_len;

  if (offset + length > queue->length) {
    return NULL;
  }
  sub_array = malloc (length * sizeof (uint8_t));
  if (sub_array) {
    copy_head = (queue->head + offset) % queue->size;
    first_copy_len = queue->size - copy_head;
    if (first_copy_len < length) {
      memcpy (sub_array, queue->array + copy_head, first_copy_len);
      memcpy (sub_array + first_copy_len, queue->array, length - first_copy_len);
    } else {
      memcpy (sub_array, queue->array + copy_head, length);
    }
  }
  return sub_array;
} 

void queue_add (queue_t* queue, uint8_t byte) {
  int tail = (queue->head + queue->length) % queue->size;
  queue->array[tail] = byte;
  if (queue->length < queue->size) {
    queue->length += 1; 
  } else {
    queue->head = (queue->head + 1) % queue->size;
  }
}

int queue_pop (queue_t *queue, uint8_t *element) {
  if (queue->length <= 0) {
    return -1;
  }
  *element = queue->array[queue->head];
  queue->head = (queue->head + 1) % queue->size;
  queue->size -= 1;
  return 0;
}
