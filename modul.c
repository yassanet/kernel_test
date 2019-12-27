#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>

MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Me");
MODULE_LICENSE("GPL");

struct task_struct *k;

struct timer_list mytimer;

static unsigned int test_major;
static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_NUM  = 2;
static struct cdev mydevice_cdev[2];
static char device_buf[101];

DECLARE_WAIT_QUEUE_HEAD(wait_queue_etx);
int wait_queue_flag = 0;
int read_count = 0;

static struct test_sysctl_settings {
	char    info[16];
	char    string[16];
	int     action;
	int     count;
} test_sysctl_settings;


#define MYTIMER_TIMEOUT_SECS    10


struct files_stat_struct files_stat2 = {
        .max_files = 10
};


static int test_sysctl_handler(struct ctl_table *ctl, int write,
				void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret=0;
	
        pr_info("%s buffer %s\n", __func__, (char *)buffer);
	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);

        return ret;
}

static int test_sysctl_info(struct ctl_table *ctl, int write,
                           void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret=0;
        pr_info("%s buffer %s\n", __func__, (char *)buffer);
	ret = proc_dostring(ctl, write, buffer, lenp, ppos);

	if (write) {
        	pr_info("%s string %s\n", __func__, test_sysctl_settings.string);
		//proc_sys_poll_notify(ctl->poll);
	}

	return ret;
}

static DEFINE_CTL_TABLE_POLL(action_poll);

static struct ctl_table test_table[] = {
	{
		.procname       = "info",
		.data           = &test_sysctl_settings.info,
		.maxlen         = 16,
		.mode           = 0444,
		.proc_handler   = test_sysctl_info,
		.poll		= &action_poll,
	},
	{
		.procname       = "action",
		.data           = &test_sysctl_settings.action,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = test_sysctl_handler,
	},
	{
		.procname       = "string",
		.data           = &test_sysctl_settings.string,
		.maxlen         = sizeof(test_sysctl_settings.string),
		.mode           = 0644,
		.proc_handler   = test_sysctl_info,
	},
	{
		.procname       = "count",
		.data           = &test_sysctl_settings.count,
		.maxlen         = sizeof(int),
		.mode           = 0444,
		.proc_handler   = test_sysctl_handler,
	},

	{ }
};
static struct ctl_table mod_test_table[] = {
        {
                .procname       = "test",
                .maxlen         = 0,
                .mode           = 0555,
                .child          = test_table,
        },
        { }
};

/* Make sure that /proc/sys/dev is there */
static struct ctl_table mod_root_table[] = {
        {
                .procname       = "mod",
                .maxlen         = 0,
                .mode           = 0555,
                .child          = mod_test_table,
        },
        { }
};


/* Timer */
static void mytimer_fn(struct timer_list *data)
{
        printk(KERN_ALERT "10 secs passed.\n");
	test_sysctl_settings.count++;
        mod_timer(&mytimer, jiffies + 1000);
}


/* Thread */
//static void kthread_main(void)
//{
//        pr_info("loop\n");
//	mdelay(5000);
//}

static int kthread_func(void* arg)
{
	printk(KERN_INFO "[%s] start kthread\n", k->comm);

	while (1) {
		printk(KERN_INFO "Waiting For Event...\n");
		wait_event_interruptible(wait_queue_etx, wait_queue_flag != 0 );
		if(wait_queue_flag == 2) {
			printk(KERN_INFO "Event Came From Exit Function\n");
			return 0;
		}
		printk(KERN_INFO "Event Came From Read Function - %d\n", ++read_count);
		wait_queue_flag = 0;
	}
	do_exit(0);

	printk(KERN_INFO "[%s] stop kthread\n", k->comm);

	return 0;
}

/* device */
static int mydevice_open1(struct inode *inode, struct file *file)
{
	printk("mydevice_open1 inode=%p file=%p", inode, file);
	file->private_data = &device_buf[0];
	return 0;
}

static int mydevice_close1(struct inode *inode, struct file *file)
{
	printk("mydevice_close1");
	return 0;
}

static ssize_t mydevice_read1(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	char *data = filp->private_data;
	char *tmp_buf = kmalloc(count, GFP_KERNEL);
	int i;

	printk("mydevice_read1 file=%p count=%ld", filp, count);

	for (i = 0; i < count; i++) {
		if (data[i] == -1) {
			break;
		}
		tmp_buf[i] = data[i];
	}
	//if (copy_to_user(buf, tmp_buf, i)) {
	if (copy_to_user(buf, tmp_buf, count)) {
		kfree(tmp_buf);
		return -EFAULT;
	}
	filp->private_data=&data[i];
	kfree(tmp_buf);

	wait_queue_flag = 1;
	wake_up_interruptible(&wait_queue_etx);

	return i;
}

static ssize_t mydevice_write1(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk("mydevice_write1");
	return 1;
}
static int mydevice_open2(struct inode *inode, struct file *file)
{
	printk("mydevice_open2");
	return 0;
}

static int mydevice_close2(struct inode *inode, struct file *file)
{
	printk("mydevice_close2");
	return 0;
}

static ssize_t mydevice_read2(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk("mydevice_read2");
	buf[0] = 'B';
	return 1;
}

static ssize_t mydevice_write2(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk("mydevice_write2");
	return 1;
}

static const struct file_operations s_mydevice_fops[2] = {
	{
		.open    = mydevice_open1,
		.release = mydevice_close1,
		.read    = mydevice_read1,
		.write   = mydevice_write1,
	},
	{
		.open    = mydevice_open2,
		.release = mydevice_close2,
		.read    = mydevice_read2,
		.write   = mydevice_write2,
	},
};


static struct ctl_table_header *test_sysctl_header;
static int dummy_init(void)
{
	dev_t first;
	int i, c;

        pr_info("Hi\n");

	/* sysctl and procfs */
	sprintf(test_sysctl_settings.info, "info"); 
	sprintf(test_sysctl_settings.string, "test"); 
	test_sysctl_settings.count = 0;
	test_sysctl_header = register_sysctl_table(mod_root_table);

	/* Timer */
        timer_setup(&mytimer, mytimer_fn, 0);
        mod_timer(&mytimer, jiffies + 1);

	/* charactor device(dynamic alloc) */
	alloc_chrdev_region(&first, MINOR_BASE, MINOR_NUM, "/dev/modtest");
        pr_info("char dev: %d, %d\n", MAJOR(first), MINOR(first));
	test_major = MAJOR(first);

	for(i = 0; i < 100; i++) {
		device_buf[i] = (char)i;
	}
	device_buf[i] = -1;
	//cdev_init(&mydevice_cdev, &s_mydevice_fops);
	//mydevice_cdev.owner = THIS_MODULE;
	//cdev_add(&mydevice_cdev, MKDEV(test_major, MINOR_BASE), MINOR_NUM);
	for(c = 0; c < MINOR_NUM; c++) {
		cdev_init(&mydevice_cdev[c], &s_mydevice_fops[c]);
		mydevice_cdev[c].owner = THIS_MODULE;
		cdev_add(&mydevice_cdev[c], MKDEV(test_major, c), 1);
	}
	
	/* wait queue */
	init_waitqueue_head(&wait_queue_etx);
	/* thread */
	k = kthread_run(kthread_func, NULL, "testmod kthread");

        return 0;
}

static void dummy_exit(void)
{

	/* wait queue */
	wait_queue_flag = 2;
	wake_up_interruptible(&wait_queue_etx);

	/* thread */
	kthread_stop(k);
	
	cdev_del(&mydevice_cdev[0]);
	cdev_del(&mydevice_cdev[1]);
	unregister_chrdev_region(MKDEV(test_major, MINOR_BASE), MINOR_NUM);

	/* Timer */
	del_timer(&mytimer);

	/* sysctl and procfs */
	if (test_sysctl_header)
		unregister_sysctl_table(test_sysctl_header);


        pr_info("Bye\n");
}

module_init(dummy_init);
module_exit(dummy_exit);
