#if !defined(__BUFFER_H__)
#define __BUFFER_H__

/*
 * --------------------------------------------------------------
 * No tweakables here! See buffer_opt.h and buffer.c for options.
 * --------------------------------------------------------------
 */

#if BUFFER_SNPRINTF	/* We defer inclusion here for 'byte'. */
#include "buffer_i.h"
#define sz(x)	((x)->data)	/* Handy if you switch back and forth. */
#else
#define sz(x)	(x)
#endif

/*
 * Handle GCC-isms.
 */
#if !defined(__GNUC__)
#define __attribute__(x)
#define __FUNCTION__	__FILE__
#endif

/*
 * These macros neatly pass the required information without breaking the
 * rest of the code.  The 'get_buffer()' code should be used for a temporary
 * memory chunk such as the current CircleMUD 'buf,' 'buf1,' and 'buf2'
 * variables.  Remember to use 'release_buffer()' to give up the requested
 * buffer when finished with it.  'release_my_buffers()' may be used in
 * functions with a lot of return statements but it is _not_ encouraged.
 * 'get_memory()' and 'release_memory()' should only be used for memory that
 * you always want handled here regardless of BUFFER_MEMORY.
 */
#define get_buffer(a)		acquire_buffer((a), BT_STACK, NULL, __FUNCTION__, __LINE__)
#define get_memory(a)		acquire_buffer((a), BT_MALLOC, NULL, __FUNCTION__, __LINE__)
#define release_buffer(a)	do { detach_buffer((a), BT_STACK, __FUNCTION__, __LINE__); (a) = NULL; } while(0)
#define release_memory(a)	do { detach_buffer((a), BT_MALLOC, __FUNCTION__, __LINE__); (a) = NULL; } while(0)
#define release_my_buffers()	detach_my_buffers(__FUNCTION__, __LINE__)

/*
 * Types for the memory to allocate.  It should never be necessary to use
 * these definitions directly.
 */
#define BT_STACK	0	/* Stack type memory.			*/
#define BT_PERSIST	1	/* A buffer that doesn't time out.	*/
#define BT_MALLOC	2	/* A malloc() memory tracker.		*/

/*
 * Public functions for outside use on buffer passing.
 */
#if BUFFER_SNPRINTF
int bprintf(buffer *buf, const char *format, ...);
buffer *b2b_cpy(buffer *str1, const buffer *str2);
buffer *s2b_cpy(buffer *str1, const char *str2);
char *b2s_cpy(char *str1, const buffer *str2);
char *b2s_cat(char *str1, const buffer *str2);
buffer *s2b_cat(buffer *str1, const char *str2);
buffer *b2b_cat(buffer *str1, const buffer *str2);
#else
#define bprintf sprintf
#define b2b_cpy strcpy
#define s2b_cpy strcpy
#define b2s_cpy strcpy
#define b2s_cat strcat
#define s2b_cat strcat
#define b2b_cat strcat
#endif

/*
 * External functions required for memory tracking.
 */
#if BUFFER_MEMORY
void *debug_calloc(size_t number, size_t size, const char *varname, const char *func, int line);
void *debug_realloc(void *ptr, size_t size, const char *varname, const char *func, int line);
void debug_free(void *ptr, const char *func, ush_int line);
buffer *debug_str_dup(const char *txt, const char *varname, const char *func, ush_int line);
#endif

/*
 * Generic prototypes.
 */
void init_buffers(void);
void exit_buffers(void);
void release_all_buffers(int pulse);
struct buf_data *detach_buffer(buffer *data, byte type, const char *func, const int line_n);
void detach_my_buffers(const char *func, const int line_n);
buffer *acquire_buffer(size_t size, int type, const char *varname, const char *who, ush_int line);
void show_buffers(struct char_data *ch, int buffer_type, int display_type);

/*
 * Cache reporting, for act.wizard.c: do_show().
 */
int buffer_cache_stat(int type);
#define BUFFER_CACHE_HITS	0
#define BUFFER_CACHE_MISSES	1

#endif
