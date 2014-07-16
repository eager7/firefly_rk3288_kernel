#include <linux/module.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include "firefly_vga.h"

static int firefly_vga_dbg_level = 1;
module_param_named(dbg_level, firefly_vga_dbg_level, int, 0644);

#define DBG( fmt, arg...) do {            \
    if (firefly_vga_dbg_level)                     \
    printk(KERN_WARNING"LT8631A...%s: " fmt , __FUNCTION__, ## arg); } while (0)


struct firefly_vga_info vga_info;
extern int firefly_register_display_vga(struct device *parent);


static int firefly_vga_initial(void)
{
	struct rk_screen screen;
	
	// RK1000 tvencoder i2c reg need dclk, so we open lcdc.
	memset(&screen, 0, sizeof(struct rk_screen));
	
	/* screen type & face */
	screen.type = SCREEN_RGB;
	screen.face = OUT_P888;
	
	/* Screen size */
	screen.mode.xres = 1280;
	screen.mode.yres = 720;
	/* Timing */
	screen.mode.pixclock = 74250000;
	screen.mode.refresh = 60;
	screen.mode.left_margin = 116;
	screen.mode.right_margin = 16;
	screen.mode.hsync_len = 6;
	screen.mode.upper_margin = 25;
	screen.mode.lower_margin = 14;
	screen.mode.vsync_len = 6;
	
	rk_fb_switch_screen(&screen, 2 , vga_info.video_source);
	

	return 0;
}


static void firefly_early_suspend(struct early_suspend *h)
{
	printk("firefly_early_suspend\n");
	if(vga_info.vga) {
		vga_info.vga->ddev->ops->setenable(vga_info.vga->ddev, 0);
		vga_info.vga->suspend = 1;
	}
	return;
}

static void firefly_early_resume(struct early_suspend *h)
{
	printk("firefly vga exit early resume\n");
	if(vga_info.vga) {
		vga_info.vga->suspend = 0;
		rk_display_device_enable((vga_info.vga)->ddev);
	}
	return;
}

static int firefly_fb_event_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct fb_event *event = data;
	int blank_mode = *((int *)event->data);
	
	printk("%s %d (action==FB_EARLY_EVENT_BLANK %d)  (action == FB_EVENT_BLANK %d) \n",__FUNCTION__,__LINE__,(action == FB_EARLY_EVENT_BLANK), (action == FB_EVENT_BLANK));
	
	if (action == FB_EARLY_EVENT_BLANK) {
	    printk("%s %d (blank_mode==FB_BLANK_UNBLANK %d)\n",__FUNCTION__,__LINE__,blank_mode==FB_BLANK_UNBLANK );
		switch (blank_mode) {
			case FB_BLANK_UNBLANK:
				break;
			default:
				firefly_early_suspend(NULL);
				break;
		}
	}
	else if (action == FB_EVENT_BLANK) {
    printk("%s %d (blank_mode==FB_BLANK_UNBLANK %d)\n",__FUNCTION__,__LINE__,blank_mode==FB_BLANK_UNBLANK );
		switch (blank_mode) {
			case FB_BLANK_UNBLANK:
				firefly_early_resume(NULL);
				break;
			default:
				break;
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block firefly_fb_notifier = {
        .notifier_call = firefly_fb_event_notify,
};

static int firefly_vga_probe(struct platform_device *pdev)
{
    int ret = -1;
    int gpio, rc,flag;
	struct device_node *vga_node = pdev->dev.of_node;

    memset(&vga_info, 0, sizeof(struct firefly_vga_info));

    vga_info.pdev = pdev;

	gpio = of_get_named_gpio_flags(vga_node,"gpio-pwn", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid gpio-pwn: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "vga_pwn");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	vga_info.gpio_pwn = gpio;
	vga_info.gpio_pwn_enable = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(vga_info.gpio_pwn, vga_info.gpio_pwn_enable);
	
	gpio = of_get_named_gpio_flags(vga_node,"gpio-sel", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid gpio-sel: %d\n",gpio);
		return -1;
	} 
   ret = gpio_request(gpio, "vga_sel");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	vga_info.gpio_sel = gpio;
	vga_info.gpio_sel_enable = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(vga_info.gpio_sel, vga_info.gpio_sel_enable);

	
	of_property_read_u32(vga_node, "rockchip,source", &(rc));
	vga_info.video_source = rc;
	of_property_read_u32(vga_node, "rockchip,prop", &(rc));
	vga_info.property = rc - 1;
	
	vga_info.mode = 1;
	
	//firefly_vga_initial();
	
	firefly_register_display_vga(&pdev->dev);
	
	//fb_register_client(&firefly_fb_notifier);
	
	printk("%s %d\n",__FUNCTION__,__LINE__);
	return 0;  //return Ok

failed_1:
	return ret;
}

static int firefly_vga_remove(struct platform_device *pdev)
{ 
    return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id of_rk_firefly_vga_match[] = {
	{ .compatible = "firefly,vga" },
	{ /* Sentinel */ }
};
#endif

static struct platform_driver firefly_vga_driver = {
	.probe		= firefly_vga_probe,
	.remove		= firefly_vga_remove,
	.driver		= {
		.name	= "firefly-vga",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_rk_firefly_vga_match,
#endif
	},

};


static int __init firefly_vga_init(void)
{
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
    return platform_driver_register(&firefly_vga_driver);
}

static void __exit firefly_vga_exit(void)
{
	platform_driver_unregister(&firefly_vga_driver);
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
}


MODULE_AUTHOR("teefirefly@gmail.com");
MODULE_DESCRIPTION("Firefly-RK3288 VGA driver");
MODULE_LICENSE("GPL");

late_initcall(firefly_vga_init);
module_exit(firefly_vga_exit);


