#include <linux/list.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>  
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/of.h>
#include <linux/of_device.h>


//#include <linux/drm.h>
#include <drm/drm_device.h>
#include <drm/drm_crtc.h>
#include <drm/drm_vblank.h>
#include <drm/drmP.h>
#include <drm/drm_connector.h>
#include <drm/drm_mode.h>

#include "dispatcher.h"
#include "data_queue.h"
#include "window.h"
#include "event.h"
#include "vsync.h"

struct drm_device * drm_dev = NULL;
//static DECLARE_WAIT_QUEUE_HEAD(vsync_wait_queue);
//static bool vsync_received = false;

#define LOG_TAG	 "dispatcher:"
#define print_dbg(fmt, ...) printk(KERN_DEBUG LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)
#define print_err(fmt, ...) printk(KERN_ERR LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)
#define print_info(fmt, ...) printk(KERN_NOTICE LOG_TAG "%s:%d->" fmt "\n", \
					__func__, __LINE__, ##__VA_ARGS__)


//static void vsync_callback(struct drm_device *dev, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec);
//static int init_vsync_monitoring(struct drm_device *dev);
//static long wait_for_vsync_event(unsigned int timeout_ms);
int drm_init(void);
void dispatch_vsync(void);
//ssize_t get_vsync(void);
void print_time(void);

//static void vsync_callback(struct drm_device *dev, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec)
//{
//    // Set the vsync flag and wake up any processes waiting for the vsync event
//    drm_handle_vblank(dev, 0);
//    print_dbg("current counter is %d", dev->dev->vblank[0].count);
    //vsync_received = true;
    //wake_up(&vsync_wait_queue);
//    dispatch_vsync();
//}


static int my_probe(void)
{
    struct device_node *node, *child;
    //const char *compatible;
    struct property * prop;
    int ret = 0;
    //struct device_node *drm_node = drm_dev->dev->of_node;

    //node = of_find_node_by_name(NULL, "chosen");
    //node = of_root;
    //if (!of_node_is_root(node)){
    //  print_err("not find root!");
    //  return -1;
    //}
    //node = drm_node;
    node = of_find_node_by_name(NULL, "display-subsystem");
    if (!node) {
      print_err("not find node!");
      return -1;
    }
    drm_dev = drm_device_get_by_name("rockchip");
    if (!drm_dev) {
      print_err("not find drm_dev!");
      return -1;
    }

    for_each_child_of_node(node, child) {
        for_each_property_of_node(child, prop){
          print_dbg("name:%s", prop->name);
          print_dbg("length:%d", prop->length);
          print_dbg("data:%s", (char *) prop->value);
        }
    }

    return ret;
}


void print_time(void)
{
  //struct timex  txc;
  //struct rtc_time tm;
  //do_gettimeofday(&(txc.time));
  //rtc_time_to_tm(txc.time.tv_sec,&tm);
  //printk("UTC time :%d-%d-%d %d:%d:%d /n",tm.tm_year+1900,tm.tm_mon, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
  ktime_t now = ktime_get();
  s64 microseconds = ktime_to_ns(now) / 1000;
  printk(KERN_INFO "Current microseconds: %lld\n", microseconds);
}



// Initialize vsync monitoring

//static int init_vsync_monitoring(struct drm_device *dev)
//{
    //int ret;
    //struct drm_crtc * crtc = list_first_entry(&(dev)->mode_config.crtc_list, struct drm_crtc, head);
    // Set up the vsync callback
    //ret = drm_vblank_init(dev, 1); // The second argument is the number of CRTCs (display controllers)
    //if (ret) {
    //    printk(KERN_ERR "Failed to initialize vblank: %d\n", ret);
    //    return ret;
    //}
    //drm_crtc_vblank_on(crtc);

    // Register the vsync callback function
    //drm_vblank_get(dev, 0); // The second argument is the CRTC index
    //drm_handle_vblank(dev, 0);
    //dev->vblank[0].enabled = true;
    //dev->vblank[0].count = 0;
    //dev->vblank[0].last = 0;
    //dev->vblank[0].sequence = 0;
    //dev->vblank[0].max_vblank_count = 1;
    //dev->vblank[0].queue = vsync_wait_queue;
    //dev->vblank[0].event = vsync_callback;

    //return 0;
//}

// Wait for a vsync event
//static long wait_for_vsync_event(unsigned int timeout_ms)
//{
//    long ret;
//
//    ret = wait_event_interruptible_timeout(vsync_wait_queue, vsync_received, msecs_to_jiffies(timeout_ms));
//    if (ret > 0) {
//        vsync_received = false; // Reset the vsync flag
//        return 0; // Vsync event received
//    } else if (ret == 0) {
//        return -ETIMEDOUT; // Timeout
//    } else {
//        return ret; // Interrupted by a signal or error
//    }
//}

int drm_init(void)
{
    //struct file *file;
    //struct inode *inode;
    //struct drm_device* dev;
    int ret; 
    struct drm_crtc *crtc;
    //print_dbg("step 1");
    // Open the DRM device
    //file = filp_open("/dev/dri/card0", O_RDWR | O_CLOEXEC, 0);
    //if (IS_ERR(file)) {
    //    printk(KERN_ERR "Failed to open DRM file: %ld\n", PTR_ERR(file));
    //    return PTR_ERR(file);
    //}
    //drm_dev = file->private_data;
    //inode = file_inode(file);
    //if (!inode) {
    //  printk(KERN_ERR "inode is null\n");
    //  return -EFAULT;
    //}
    //drm_dev = container_of(inode->i_private, struct drm_device, anon_inode);
    //if (!dev) {
    //  printk(KERN_ERR "dev is null\n");
    //  return -EFAULT;
    //}
    //if (copy_from_user(drm_dev, (struct drm_device __user *)dev, sizeof(struct drm_device)))
    //  printk(KERN_ERR "copy fomr user failed\n");
    //  return -EFAULT;
    //if (!drm_dev) {
    //  printk(KERN_ERR "Failed to get drm_device\n");
    //  return -ENODEV;
    //}
    //print_dbg("step 2");
    
    ret = my_probe();
    if (ret) {
        printk(KERN_ERR "Failed to probe: %d\n", ret);
        return ret;
    }
    
    ret = drm_vblank_init(drm_dev, drm_dev->mode_config.num_crtc);
	  if (ret) {
        printk(KERN_ERR "Failed to init vblank: %d\n", ret);
        return ret;
    }
    //print_dbg("step 3");
    /*
	  * enable drm irq mode.
	  * - with irq_enabled = true, we can use the vblank feature.
	  */
	  drm_dev->irq_enabled = true;
    drm_for_each_crtc(crtc, drm_dev){
      printk(KERN_INFO "CRTC ID: %d\n", crtc->base.id);
      printk(KERN_INFO "CRTC name: %s\n", crtc->name);
      printk(KERN_INFO "CRTC enabled: %d\n", crtc->enabled);
      printk(KERN_INFO "CRTC no_vblank: %d\n", crtc->state->no_vblank);
    }
    //ret = drm_vblank_enable(drm_dev, 0);
    //if (ret) {
    //    printk(KERN_ERR "Failed to enable vblank in crtc 0: %d\n", ret);
    //    return ret;
    //}
    
    //filp_close(file, file->f_inode);
    //print_dbg("step 3");
    return 0;
}

void dispatch_vsync(void)
{
  u64 seq;
  //struct drm_vblank_crtc *vblank;
  struct drm_crtc *crtc;

  print_time();
  //seq = drm_vblank_count(drm_dev, 0);
  if (!drm_dev){
    print_err("drm_dev is null!");
    return;
  }
  //vblank = &drm_dev->vblank[0];
  //if (!vblank){
  //  print_err("vblank is null!");
  //  return;
  //}
  //seq = vblank->count;
  //print_dbg("before wait, vblank_count is %llu", seq);

  //drm_wait_one_vblank(drm_dev, 1);
  drm_for_each_crtc(crtc, drm_dev){
      if(crtc->enabled){
        seq = drm_crtc_vblank_count(crtc);
        print_dbg("before wait, vblank_count is %llu", seq);

        drm_crtc_wait_one_vblank(crtc);
        
        seq = drm_crtc_vblank_count(crtc);
        print_dbg("after wait, vblank_count is %llu", seq);
      }
  }
  //seq = vblank->count;
  //print_dbg("after wait, vblank_count is %llu", seq);
  print_time();

  //window* win;
	//struct list_head* window_list;
  //int ret;
  //struct drm_pending_vblank_event *e;
  //struct drm_framebuffer *fb;
  //uint64_t vblank_sequence;
  //struct drm_crtc *crtc = drm_crtc_from_index(drm_dev, 0);
  //unsigned int flags = DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK;

  //fb = kzalloc(sizeof(struct drm_framebuffer), GFP_KERNEL);
  //e = kzalloc(sizeof(struct drm_pending_vblank_event), GFP_KERNEL);;
  //if (!fb) {
    // Handle allocation failure
  //  printk(KERN_ERR "Failed to init fb\n");
  //}
  //e = list_first_entry(&drm_dev->vblank_event_list, struct drm_pending_vblank_event, list_head);
  //print_dbg("before page flip, sequnce number is %llu", e->);
  //ret = crtc->funcs->page_flip(crtc, fb, e, flags, NULL);
  //if (ret) {
  //  printk(KERN_ERR "Failed to do page flip: %d\n", ret);
  //  return;
    // Handle error
  //}
  //print_dbg("after page flip, sequnce number is %llu", vblank_sequence);

  //ret = crtc->funcs->page_flip(crtc, fb, e, flags, NULL);
  //if (ret) {
  //  printk(KERN_ERR "Failed to do page flip: %d\n", ret);
  //  return;
    // Handle error
  //}
  //print_dbg("after page flip, sequnce number is %llu", vblank_sequence);

  // Wait for the vblank event
  //wait_event(drm_dev->vblank_event_list, !list_empty(&drm_dev->vblank_event_list));
  //if (list_empty(&drm_dev->vblank_event_list)) {
  //  printk(KERN_ERR "event list is empty: %d\n", ret);
  //  return;
    // Handle error
  //}

  // Retrieve the vblank sequence number from the event
  //pev = list_first_entry(&drm_dev->vblank_event_list, struct drm_pending_vblank_event, link);
  //vblank_sequence = pev->sequence;
  
  
  //print_time();
	//drm_wait_one_vblank(drm_dev, 0);
  //print_time();

  //window_list = get_window_list();
	//list_for_each_entry(win, window_list, node){
  //  if (win->vsync_tick < 100){
  //    win->vsync_tick++;
  //  }
  //  else{
  //    win->vsync_tick=0;
  //  }
	//}
}

//ssize_t get_vsync(void) 
//{
//    int ret;
//    // Wait for a vsync event
//    ret = wait_for_vsync_event(1000); // Wait for up to 1000 milliseconds (1 second) for a vsync event
//    if (ret == 0) {
//        // Vsync event received
//    } else if (ret == -ETIMEDOUT) {
//        // Timeout occurred
//    } else {
//        // Handle error or interruption
//    }
//    return 0; 
//}
