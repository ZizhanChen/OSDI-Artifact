#include <linux/list.h>
#include "window.h"
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

#define LOG_TAG	 "window:"
#define print_dbg(fmt, ...) printk(KERN_DEBUG LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)
#define print_err(fmt, ...) printk(KERN_ERR LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)
#define print_info(fmt, ...) printk(KERN_NOTICE LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)

//struct list_head window_list;
LIST_HEAD(window_list);

DEFINE_SPINLOCK(window_list_lock);

#define USE_LOCK 1
#define lock_share()  do {if(USE_LOCK) spin_lock(&window_list_lock);} while(0)
#define unlock_share()  do {if(USE_LOCK) spin_unlock(&window_list_lock);} while(0)

//void init_window_list(void)
//{
//    INIT_LIST_HEAD(&window_list);
//}
//EXPORT_SYMBOL(init_window_list);

void add_window(long pid, int left, int top, int height, int width, int zorder, char * event_buffer) 
{
    window * win;
    lock_share();
    win = kzalloc(sizeof(window), GFP_KERNEL);
    win->pid = pid; 
    win->vsync_tick = 0;
    win->left = left;
    win->top = top;
    win->height = height;
    win->width = width;
    win->zorder = zorder;
    win->event_buffer = event_buffer;
    list_add(&win->node, &window_list);
    print_dbg("---------- window debug -----------");
	print_dbg("window pid = %ld", win->pid);
	print_dbg("left = %d, top = %d, height = %d, width=%d", win->left, win->top, win->height, win->width);
	print_dbg("zorder = %d", win->zorder);
	print_dbg("---------------------------------------");
    unlock_share(); 
}
//EXPORT_SYMBOL(add_window);

void del_window(long pid)
{
    window *win = NULL;
    lock_share();
    list_for_each_entry(win, &window_list, node) {
        if (win->pid == pid){
            list_del(&win->node);
        }
    }
    unlock_share(); 
}
//EXPORT_SYMBOL(del_window);

window* get_window(long pid)
{
    window *win = NULL;
    lock_share();
    list_for_each_entry(win, &window_list, node) {
        if (win->pid == pid){
            unlock_share();
            return win;
        }
    }
    unlock_share();
    return NULL;
}
//EXPORT_SYMBOL(get_window);

void set_window(long pid, int left, int top, int height, int width, int zorder, char * event_buffer)
{
    window *win = NULL;
    lock_share();
    list_for_each_entry(win, &window_list, node) {
        if (win->pid == pid){
            win->left = left;
            win->top = top;
            win->height = height;
            win->width = width;
            win->zorder = width;
            win->event_buffer = event_buffer;
        }
    }
    unlock_share();
}
//EXPORT_SYMBOL(set_window);

window* get_first_window(){
    window *win = NULL;
    win = list_first_entry_or_null(&window_list, struct window, node);
    return win;
}

window* get_next_window(window* win){
    return list_next_entry(win, node);
}

struct list_head* get_window_list(){
    return &window_list;
}