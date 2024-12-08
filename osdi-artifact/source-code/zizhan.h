#ifndef __ZIZHAN_H__
#define __ZIZHAN_H__

#include <linux/types.h>



//static int hello_init(void);
//static void hello_exit(void);
ssize_t zizhan_read (char * buff, size_t length);
ssize_t zizhan_dump (size_t length);
ssize_t zizhan_write (const char* data, size_t length);
void zizhan_add_window(long pid, int left, int top, int height, int width, int zorder, char * event_buffer);
void zizhan_del_window(long pid);
void zizhan_dispatch_once(void);
void zizhan_vsync_init(void);
void zizhan_get_vsync(void);


#endif