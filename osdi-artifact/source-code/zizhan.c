#include <linux/init.h>
#include <linux/kernel.h>  
#include <linux/module.h>
#include <linux/list.h>

#include "data_queue.h"
#include "zizhan.h"
#include "window.h"
#include "dispatcher.h"
#include "vsync.h"

DECLARE_DATA_QUEUE(zizhan_data, 1000);
 
//typedef ssize_t (*zizhan_write_t)(const char* data, size_t length);
//typedef ssize_t (*zizhan_dump_t)(size_t length);

//void register_zizhan_write_handler(zizhan_write_t handler);
//void register_zizhan_dump_handler(zizhan_dump_t handler);

//static int hello_init(void)
//{ 
//    printk("Hello! This is the helloworld module!\n");
    //register_zizhan_write_handler(zizhan_write);
    //register_zizhan_dump_handler(zizhan_dump);
    //init_window_list();
//    return 0;
//} 
 
//static void hello_exit(void)
//{
//    printk("Module exit! Bye Bye!\n");
    //register_zizhan_write_handler(NULL);
    //register_zizhan_write_handler(NULL);
//    return;
//}


//ssize_t zizhan_read (char __user * buff, size_t length)
//{
//	if(!zizhan_data.exist_data(&zizhan_data.common)) return 0;
//	return zizhan_data.dequeue(&zizhan_data.common, buff, length);
//}

ssize_t zizhan_dump (size_t length)
{
	if(!zizhan_data.exist_data(&zizhan_data.common)) return 0;
	return zizhan_data.dump(&zizhan_data.common, length);
}

ssize_t zizhan_write (const char* data, size_t length)
{
	return zizhan_data.enqueue(&zizhan_data.common, data, length, 100);
}

void zizhan_add_window(long pid, int left, int top, int height, int width, int zorder, char * event_buffer)
{
    add_window(pid, left, top, height, width, zorder, event_buffer);
}

void zizhan_del_window(long pid)
{
    del_window(pid);
}

void zizhan_dispatch_once(void){
    dispatch_once(&zizhan_data.common);
}

void zizhan_vsync_init(void){
    drm_init();
}

void zizhan_dispatch_vsync(void){
    dispatch_vsync();
}

//EXPORT_SYMBOL(zizhan_write);
//EXPORT_SYMBOL(zizhan_dump);
 
//module_init(hello_init);
//module_exit(hello_exit);
//MODULE_LICENSE("GPL");