#ifndef SIMPLIFIED_LZ77_H
#define SIMPLIFIED_LZ77_H

/* A structure to help read bit-level contents from a file
 * The structure takes ownership of the opened file when created
 */
typedef struct bit_stream {
  FILE *file;

  /* The position of the next bit to read in a byte:
   * If it's a 0 then the next bit is the the MSB of the next byte in stream.
   * Otherwise it's the bit_pos-th MSB of last_byte.
   */
  uint8_t bit_pos;
  uint8_t last_byte;

  /* Size of file in bytes and number of bytes read.
   */
  uint64_t file_size;
  uint64_t read;
} bit_stream_t;

bit_stream_t* bit_stream_new (FILE *file);
void bit_stream_destroy (bit_stream_t **stream_ptr);
int read_1bit (bit_stream_t *stream, uint8_t *result);
int read_4bits (bit_stream_t *stream, uint8_t *result);
int read_8bits (bit_stream_t *stream, uint8_t *result);
int read_12bits (bit_stream_t *stream, uint16_t *result);



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

#endif
