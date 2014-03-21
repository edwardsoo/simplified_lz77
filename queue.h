#ifndef QUEUE_H
#define QUEUE_H

/* A fixed size queue of bytes:
 * Queueing more element than size will overwrite the oldest element
 */
typedef struct queue {
  uint8_t *array;
  int length, size, head;
} queue_t;
queue_t* queue_new (int size);
void queue_destroy (queue_t **queue_ptr);
uint8_t* queue_sub_array (queue_t *queue, int offset, int length);
void queue_add (queue_t* queue, uint8_t byte);
int queue_pop (queue_t *queue, uint8_t *element);
int queue_get (queue_t *queue, int offset, uint8_t *element);

#endif
