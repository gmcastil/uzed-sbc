#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/printk.h>

#define BASIC_MODNAME		"basic"

static int basic_probe(struct platform_device *pdev)
{
	pr_info("Basic module init\n");
	return 0;
}

static int basic_remove(struct platform_device *pdev)
{
	pr_info("Basic module exit\n");
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id basic_of_match[] = {
	{ .compatible = BASIC_MODNAME, },
	{ }
};
MODULE_DEVICE_TABLE(of, basic_of_match);
#endif

/* Create a platform driver for this device */
static struct platform_driver basic_platform_driver = {
	/* int (*probe)(struct platform_device *); */
	.probe		= basic_probe,
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
	.remove		= basic_remove,
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

	/* struct device_driver driver; */
	.driver = {
		.name		= BASIC_MODNAME,
		.of_match_table = of_match_ptr(basic_of_match),
	},
	/* const struct platform_device_id *id_table; */
	.id_table	= NULL,
	/* bool prevent_deferred_probe; */
	.prevent_deferred_probe	= false,

	/* bool driver_managed_dma; */
	.driver_managed_dma	= false	
};

/* Register this module as a platform device */
module_platform_driver(basic_platform_driver);

MODULE_AUTHOR("George Castillo");
MODULE_DESCRIPTION("Demonstrate module build, insertion and removal functionality");
MODULE_LICENSE("GPL");

