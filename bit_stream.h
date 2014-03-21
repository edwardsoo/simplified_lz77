#ifndef BIT_STREAM_H
#define BIT_STREAM_H

/* A structure to help read from a file at bits level
 * The structure takes ownership of the opened file when created
 */
typedef struct bit_in_stream {
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
} bit_in_stream_t;

bit_in_stream_t* bit_in_stream_new (FILE *file);
void bit_in_stream_destroy (bit_in_stream_t **stream_ptr);
int read_1bit (bit_in_stream_t *stream, uint8_t *result);
int read_4bits (bit_in_stream_t *stream, uint8_t *result);
int read_8bits (bit_in_stream_t *stream, uint8_t *result);
int read_12bits (bit_in_stream_t *stream, uint16_t *result);



/* A structure to help write to a file at bits level
 * The structure takes ownership of the opened file when created
 */
typedef struct bit_out_stream {
  FILE *file;

  /* The position of the next bit to write in buffer_byte:
   * If it's a 0 then buffer_byte does not contain any buffered bit.
   * Otherwise the next value written will overwrite buffer_byte
   * starting at the bit_pos-th MSG of buffer_byte.
   */
  uint8_t bit_pos;
  /* Store bits to be written.
   * It is written to file when all 8 bits are filled
   */
  uint8_t buffer_byte;

} bit_out_stream_t;

bit_out_stream_t* bit_out_stream_new (FILE* file);
void bit_out_stream_destroy (bit_out_stream_t **stream_ptr);
int write_1bit (bit_out_stream_t *stream, uint8_t value);
int write_4bits (bit_out_stream_t *stream, uint8_t value);
int write_8bits (bit_out_stream_t *stream, uint8_t value);
int write_12bits (bit_out_stream_t *stream, uint16_t value);

#endif
