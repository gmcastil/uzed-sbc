#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/types.h>

/* Include extra debug and information messages in the kernel ring buffer */
#define SBC_DEBUG
/* Try not to be too board specific */
#define SBC_MODNAME		"uzed-sbc,axi-bram-ctrl"
/* Name of the devices that will be created in /dev */
#define SBC_DEVICE_NAME		"bram"
/* Name of the class of device? WTF is this? */
#define SBC_CLASS_NAME		"sbc"
/* Maximum number of devices to allocate in /dev */
#define SBC_MAX_DEVS		10

static int sbc_open(struct inode *, struct file *);
static ssize_t sbc_write (struct file *, const char __user *, size_t, loff_t *);
static ssize_t sbc_read (struct file *, char __user *, size_t, loff_t *);
static int sbc_release(struct inode *, struct file *);

static struct class *sbc_class;
static struct device *sbc_device;
static void *base;
static dev_t dev_id;

static struct file_operations sbc_fops = {
	.owner		= THIS_MODULE,
	.open		= sbc_open,
	.read		= sbc_read,
	.write		= sbc_write,
	.release	= sbc_release,
};
/*
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	int (*iopoll)(struct kiocb *kiocb, struct io_comp_batch *,
			unsigned int flags);
	int (*iterate) (struct file *, struct dir_context *);
	int (*iterate_shared) (struct file *, struct dir_context *);
	__poll_t (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	unsigned long mmap_supported_flags;
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, loff_t, loff_t, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long, struct file_lock **, void **);
	long (*fallocate)(struct file *file, int mode, loff_t offset,
			  loff_t len);
	void (*show_fdinfo)(struct seq_file *m, struct file *f);
#ifndef CONFIG_MMU
	unsigned (*mmap_capabilities)(struct file *);
#endif
	ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
			loff_t, size_t, unsigned int);
	loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
				   struct file *file_out, loff_t pos_out,
				   loff_t len, unsigned int remap_flags);
	int (*fadvise)(struct file *, loff_t, loff_t, int);
	int (*uring_cmd)(struct io_uring_cmd *ioucmd, unsigned int issue_flags);
	int (*uring_cmd_iopoll)(struct io_uring_cmd *, struct io_comp_batch *,
				unsigned int poll_flags);
*/

/* Called when a process tries to open device file (e.g., `cat /dev/bram`) */
static int sbc_open(struct inode *, struct file *)
{
	pr_info("Callback open");
	return 0;
}

/* write() */

static ssize_t sbc_write (struct file *, const char __user *, size_t, loff_t *)
{
	pr_info("Callback write");
	return 0;
}

/* read() */
static ssize_t sbc_read (struct file *, char __user *, size_t, loff_t *)
{
	pr_info("Callback read");
	return 0;
}

static int sbc_release(struct inode *, struct file *)
{
	pr_info("Callback release");
	return 0;
}

/* ioctl() */

static int sbc_probe(struct platform_device *pdev)
{
	int ret;
	struct cdev *cdev = NULL;

	struct resource *res = NULL;

	/* Register a character device */
	ret = alloc_chrdev_region(&dev_id, 0, SBC_MAX_DEVS, SBC_DEVICE_NAME);
	if (ret) {
		dev_err(&pdev->dev, "could not obtain a device ID");
		return ret;
	}
#ifdef SBC_DEBUG
	dev_info(&pdev->dev, "obtained major number: %d, minor number: %d",
			MAJOR(dev_id), MINOR(dev_id));
#endif
	cdev = cdev_alloc();
	if (IS_ERR(cdev)) {
		dev_err(&pdev->dev, "could not initialize character device");
		dev_err(&pdev->dev, "cdev alloc error: %p", cdev);
		goto deregister_out;
	}
	cdev_init(cdev, &sbc_fops);
	ret = cdev_add(cdev, dev_id, 1);
	if (ret) {
		dev_err(&pdev->dev, "could not add character device");
		goto cdev_dealloc_out;
	}

	/* Create the device class */
	sbc_class = class_create(THIS_MODULE, SBC_CLASS_NAME);
	if (IS_ERR(sbc_class)) {
		dev_err(&pdev->dev, "could not create device class");
		goto cdev_dealloc_out;
	}

	/* Create the actual device */
	sbc_device = device_create(sbc_class, NULL, dev_id, NULL, SBC_DEVICE_NAME);
	if (IS_ERR(sbc_device)) {
		dev_err(&pdev->dev, "could not create device");
		goto class_destroy_out;
	}

	/* Get the device memory range from the device tree */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "could not get resource %d: ", IORESOURCE_MEM);
		return -EINVAL;
	}

	/* Map the device memory into the virtual address space */
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base)) {
		dev_err(&pdev->dev, "could not map resource: %pR", res);
		return PTR_ERR(base);
	}

	return 0;

class_destroy_out:
	class_destroy(sbc_class);

cdev_dealloc_out:

deregister_out:
	unregister_chrdev_region(dev_id, SBC_MAX_DEVS);

	/* free on failure after devm_ioremap() */
	/* void devm_iounmap(struct device *dev, void __iomem *addr) */
	return -1;
}

static int sbc_remove(struct platform_device *pdev)
{
	/* I don't know another way to store the base address of the resource
	 * that we are removing
	 */
	devm_iounmap(&pdev->dev, base);

	device_destroy(sbc_class, dev_id);
	class_destroy(sbc_class);

	unregister_chrdev_region(dev_id, SBC_MAX_DEVS);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id sbc_of_match[] = {
	{ .compatible = SBC_MODNAME, },
	{ }
};
MODULE_DEVICE_TABLE(of, sbc_of_match);
#endif

/* Create a platform driver for this device */
static struct platform_driver sbc_platform_driver = {
	/*   int (*probe)(struct platform_device *); */
	.probe		= sbc_probe,
	/*
	 * Traditionally the remove callback returned an int which however is
	 * ignored by the driver core. This led to wrong expectations by driver
	 * authors who thought returning an error code was a valid error
	 * handling strategy. To convert to a callback returning void, new
	 * drivers should implement .remove_new() until the conversion it done
	 * that eventually makes .remove() return void.
	 *
	 *   int (*remove)(struct platform_device *);
	 *   void (*remove_new)(struct platform_device *);
	 */
	.remove		= sbc_remove,
	.remove_new	= NULL,
	/*
	 * Power management which I will ignore for this device
	 *   void (*shutdown)(struct platform_device *);
	 *   int (*suspend)(struct platform_device *, pm_message_t state);
	 *   int (*resume)(struct platform_device *);
	 */
	.shutdown	= NULL,
	.suspend	= NULL,
	.resume		= NULL,
	/*   struct device_driver driver; */
	.driver = {
		.name		= SBC_MODNAME,
		.owner		= THIS_MODULE,
		.of_match_table = of_match_ptr(sbc_of_match),
	},
	/*   const struct platform_device_id *id_table; */
	.id_table	= NULL,
	/*   bool prevent_deferred_probe; */
	.prevent_deferred_probe	= false,
	/*   bool driver_managed_dma; */
	.driver_managed_dma	= false
};

/*
 * Use the kernel convenience macro which replaces the init and exit functions
 * as well as registering the driver with the Linux kernel platform subsystem
 */
module_platform_driver(sbc_platform_driver);

MODULE_AUTHOR("George Castillo");
MODULE_DESCRIPTION("Provides access to SBC BRAM resources");
MODULE_LICENSE("GPL");

