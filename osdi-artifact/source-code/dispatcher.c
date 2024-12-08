#include <linux/list.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>  

#include "dispatcher.h"
#include "data_queue.h"
#include "window.h"
#include "event.h"

#define LOG_TAG	 "dispatcher:"
#define print_dbg(fmt, ...) printk(KERN_DEBUG LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)
#define print_err(fmt, ...) printk(KERN_ERR LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)
#define print_info(fmt, ...) printk(KERN_NOTICE LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)

static bool policy(event * e, window * win);
static int deliver(event * e, window * win);
void dispatch_once(data_queue* queue);


void dispatch_once(data_queue* queue)
{
    window* win;
	event* e;
	struct list_head* window_list;
	e = dequeue_event(queue);
	window_list = get_window_list();
	list_for_each_entry(win, window_list, node){
		if (policy(e, win)) {
			deliver(e, win);
		}
	}
}

static bool policy(event * e, window * win){
	//if((e->x > win->left) && (e->x < win->left + win->width) && (e->y > win->top) && (e->y < win->top + win->height)){
	//	return true;
	//}
	if (win->zorder == 99){
		return true;
	}
	return false;
}

static int deliver(event * e, window * win)
{
	print_dbg("event [type = %d, code = %d, value = %d] delivered to window [%ld]", e->type, e->code, e->value, win->pid);
	memcpy(win->event_buffer, e, sizeof(event));
	print_dbg("the buffer content is [%s]", win->event_buffer);
	return 0;
}