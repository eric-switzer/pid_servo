#ifndef CIRCBUF_H
#define CIRCBUF_H

//! A structure for defining a circular buffer and for keeping track of its
//! state.
struct circ_buf_t {
  void *start;      //!< The start of the buffer.
  void *end;        //!< The end of the buffer.
  void *wpos;       //!< The most recently written-to point in the buffer.
  void *rpos;       //!< The last read-from point in the buffer.
  int size;         //!< The size of each buffer element.
  int len;          //!< The number of elements in the buffer.
};
struct circ_buf_t *circ_buf_alloc_and_init(int size_element, int num);
void circ_buf_init(struct circ_buf_t *cb, int size_element, int num);
void circ_buf_resize(struct circ_buf_t *cb, int num);
void circ_buf_destroy(struct circ_buf_t *cb);
void circ_buf_push_ptr(struct circ_buf_t *cb, const void *val);
//! A wrapper for circ_buf_push_ptr().
#define circ_buf_push(cb, val) circ_buf_push_ptr(cb, &(val))
void *circ_buf_pop_ptr(struct circ_buf_t *cb);
//! A wrapper for circ_buf_pop_ptr().
#define circ_buf_pop(cb, type) (*((type *)circ_buf_pop_ptr(cb)))
void *circ_buf_peek_ptr(struct circ_buf_t *cb);
void *circ_buf_get_ptr(struct circ_buf_t *cb, int history_index);
//! A wrapper for circ_buf_get_ptr().
#define circ_buf_get(cb, type, idx) (*((type *)circ_buf_get_ptr(cb, idx)))
char circ_buf_full(struct circ_buf_t *cb);
char circ_buf_empty(struct circ_buf_t *cb);
int circ_buf_len(struct circ_buf_t *cb);

#endif

