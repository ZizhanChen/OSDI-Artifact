#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <linux/types.h>
#include <linux/list.h>

typedef struct window
{
	long pid;
    int vsync_tick;
    int left;
    int top;
    int height;  
	int width;
    int zorder;
    char * event_buffer; 
    struct list_head node;
}window;

void init_window_list(void);
void add_window(long pid, int left, int top, int height, int width, int zorder, char * event_buffer);
void del_window(long pid);
window* get_window(long pid);
void set_window(long pid, int left, int top, int height, int width, int zorder, char * event_buffer);
window* get_first_window(void);
window* get_next_window(window* win);
struct list_head* get_window_list(void);


#endif
