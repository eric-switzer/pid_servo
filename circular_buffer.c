#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "circular_buffer.h"

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

// var_name The name of the buffer being allocated (for error printout).
// nmemb The number of members to allocate.
// size The size of each element.
// return: A pointer to the allocated memory.
void *safe_calloc(const char *var_name, size_t nmemb, size_t size) {
  void *ret;

  if ((ret = calloc(nmemb, size)) == NULL)
    handle_error(var_name);

  return ret;
}


// var_name The name of the buffer being allocated (for error printout).
// size The total size of the desired buffer, in bytes.
// return: A pointer to the allocated memory.
void *safe_malloc(const char *var_name, size_t size) {
  void *ret;

  if ((ret = malloc(size)) == NULL)
    handle_error(var_name);

  return ret;
}

// var_name The name of the buffer being allocated (for error printout).
// ptr The buffer to reallocate.
// size The new size of the buffer, in bytes.
// return: A pointer to the allocated memory.
void *safe_realloc(const char *var_name, void *ptr, size_t size) {
  void *ret;

  if (!size)
    return ptr;

  if ((ret = realloc(ptr, size)) == NULL)
    handle_error(var_name);

  return ret;
}

// Allocate and initialize a circular buffer.
//  size_element The size of each element in bytes.
//  num The total number of elements desired in the buffer.
// returns: A pointer to the newly-allocated buffer.
struct circ_buf_t *circ_buf_alloc_and_init(int size_element, int num) {
  struct circ_buf_t *cb =
    (struct circ_buf_t *)safe_malloc("cb", sizeof(struct circ_buf_t));

  circ_buf_init(cb, size_element, num);

  return cb;
}

// Initialize a circular buffer.
//  size_element The size of each element in bytes.
//  num The total number of elements desired in the buffer.
void circ_buf_init(struct circ_buf_t *cb, int size_element, int num) {
  if (cb == NULL)
    return;
  cb->start = safe_malloc("cb->start", size_element * num);
  cb->end = (void *)((char *)cb->start + size_element * num);
  cb->wpos = cb->start;
  cb->rpos = cb->start;
  cb->size = size_element;
  cb->len = num;
  memset(cb->start, 0, num * size_element);
}

// Resize a circular buffer.
// If the size of the buffer is reduced, data may be lost!
//  num The new size for the buffer.
void circ_buf_resize(struct circ_buf_t *cb, int num) {
  int data_len, tmp_len;
  char *tmp_buf;

  if (cb == NULL)
    return;

  // Reshuffle the data so that rpos is at the start of the buffer.
  if (cb->wpos == cb->rpos) {
    // Do nothing when there is nothing in the buffer.
    data_len = 0;
  }
  else if (cb->wpos > cb->rpos) {
    // In the simple case where the data do not wrap, just move everything all
    // at once.
    data_len = (char *)cb->wpos - (char *)cb->rpos;
    memmove(cb->start, cb->rpos, data_len);
  }
  else {
    // When the data wrap, we need a temporary buffer.
    data_len = cb->len * cb->size - ((char *)cb->rpos - (char *)cb->wpos);

    // Make a copy of the data at the start of the buffer.
    tmp_len = (char *)cb->wpos - (char *)cb->start;
    tmp_buf = (char *)safe_malloc("tmp_buf", tmp_len);
    memcpy(tmp_buf, cb->start, tmp_len);

    // Move rpos to the start of the buffer.
    memmove(cb->start, cb->rpos, data_len - tmp_len);

    // Move the copy back to the end.
    memcpy((char *)cb->start + data_len - tmp_len, tmp_buf, tmp_len);

    // Free temporary buffer.
    free(tmp_buf);
  }

  // Reallocate.
  cb->len = num;
  cb->start = safe_realloc("cb->start", cb->start, cb->size * cb->len);
  cb->end = (void *)((char *)cb->start + cb->size * cb->len);
  cb->rpos = cb->start;
  cb->wpos = (char *)cb->start + data_len;
}

void circ_buf_destroy(struct circ_buf_t *cb) {
  if (cb == NULL)
    return;
  free(cb->start);
  cb->start = cb->end = NULL;
  cb->wpos = cb->rpos = NULL;
  cb->size = 0;
  cb->len = 0;
}

// Get the number of elements in a circular buffer.
// returns: The number of elements in cb.
int circ_buf_len(struct circ_buf_t *cb) {
  return cb->len;
}

// Find out whether a circular buffer is full.
// returns: 1 if the buffer is full; 0 if there is still space.
char circ_buf_full(struct circ_buf_t *cb) {
  void *next_pos;

  if ((next_pos = (void *)((char *)cb->wpos + cb->size)) >= cb->end)
    next_pos = cb->start;

  if (next_pos == cb->rpos)
    return 1;
  else
    return 0;
}

// Find out whether a circular buffer is empty.
// returns: 1 if the buffer is empty, i.e., if any and all entries in it have
//         been popped; 0 otherwise.
char circ_buf_empty(struct circ_buf_t *cb) {
  if (cb->rpos == cb->wpos)
    return 1;
  else
    return 0;
}

// Add an element to a circular buffer. Copies with memcpy().
//  val The value to add.
void circ_buf_push_ptr(struct circ_buf_t *cb, const void *val) {
  void *new_pos;

  memcpy(cb->wpos, val, cb->size);

  // Wrap write pointer if needed.
  if ((new_pos = (void *)((char *)cb->wpos + cb->size)) >= cb->end)
    new_pos = cb->start;

  cb->wpos = new_pos;
}

// Get pointer to the oldest un-read element from a circular buffer. No
// memcpy().
// returns: (Pointer to) the oldest un-read value from the buffer, or NULL if
//         there are no new data.
void *circ_buf_pop_ptr(struct circ_buf_t *cb) {
  void *pop_pos;

  // Return NULL if there are no new data.
  if (cb->rpos == cb->wpos)
    return NULL;

  pop_pos = cb->rpos;

  // Wrap read pointer if needed.
  if ((cb->rpos = (void *)((char *)cb->rpos + cb->size)) >= cb->end)
    cb->rpos = cb->start;

  return pop_pos;
}

// Get pointer to the oldest un-read element from a circular buffer, but
// don't modify the buffer.
// returns: (Pointer to) the oldest un-read value from the buffer, or NULL if
//         there are no new data.
void *circ_buf_peek_ptr(struct circ_buf_t *cb) {

  // Return NULL if there are no new data.
  if (cb->rpos == cb->wpos)
    return NULL;

  return cb->rpos;
}

// Read an element from a circular buffer.
//
//  history_index The index of the element to read, where 0 is the most
//                      recent. If larger than circ_buf_len(), the program
//                      will terminate.
//
// returns: The requested value.
void *circ_buf_get_ptr(struct circ_buf_t *cb, int history_index) {
  void *get_pos;

  if (history_index >= circ_buf_len(cb)) {
    printf("Attempt to access ring buffer at history index %d, but "
                     "buffer length is %d.", history_index, circ_buf_len(cb));
    exit(EXIT_FAILURE);
  }

  if ((get_pos = (void *)((char *)cb->wpos - history_index * cb->size)) <
      cb->start)
    get_pos = (void *)((char *)get_pos + circ_buf_len(cb) * cb->size);

  return get_pos;
}
