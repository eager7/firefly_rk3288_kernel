#include <linux/ctype.h>
#include <linux/string.h>
#include "firefly_vga.h"
#include <linux/fb.h>
#include <linux/rk_fb.h>
#include <linux/display-sys.h>

#define E(fmt, arg...) printk("<3>!!!%s:%d: " fmt, __FILE__, __LINE__, ##arg)

static const struct fb_videomode sda7123_vga_mode[] = {
		//name			refresh		xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw	polariry	
	{	"1280x720p@60Hz",	60,	   1280,    720,	74250000,	220,	110,	 20,	  5,	 40,	  5,		  1,			1,		0	},
	{	"1920x1080p@60Hz",	60,	   1920,   1080,   148500000,	148,	88,	36,	4,	44,	5,	FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	0	},
};



static struct sda7123_monspecs vga_monspecs;


int firefly_switch_fb(const struct fb_videomode *modedb, int tv_mode)
{	
	struct rk_screen *screen;
	
	if(modedb == NULL)
		return -1;
	screen =  kzalloc(sizeof(struct rk_screen), GFP_KERNEL);
	if(screen == NULL)
		return -1;
	
	memset(screen, 0, sizeof(struct rk_screen));	
	/* screen type & face */
	screen->type = SCREEN_RGB;
	screen->face = OUT_P888;
	
	screen->mode = *modedb;
	
	/* Pin polarity */
	if(FB_SYNC_HOR_HIGH_ACT & modedb->sync)
		screen->pin_hsync = 1;
	else
		screen->pin_hsync = 0;
	if(FB_SYNC_VERT_HIGH_ACT & modedb->sync)
		screen->pin_vsync = 1;
	else
		screen->pin_vsync = 0;	
		
	screen->pin_den = 0;
	screen->pin_dclk = 1;
	
	/* Swap rule */
	screen->swap_rb = 0;
	screen->swap_rg = 0;
	screen->swap_gb = 0;
	screen->swap_delta = 0;
	screen->swap_dumy = 0;
	
	/* Operation function*/
	screen->init = NULL;
	screen->standby = NULL;	
	screen->init = NULL;
	
	rk_fb_switch_screen(screen, 1 , vga_info.video_source);
	
	gpio_set_value(vga_info.gpio_sel,vga_info.gpio_sel_enable);
	
	kfree(screen);
	
	return 0;
}


int firefly_vga_standby(int type)
{
	unsigned char val;
	int ret;
	int ypbpr = 0, cvbs = 0;
	struct rk_screen screen;
	
	if(vga_monspecs.enable)
		return 0;

	screen.type = SCREEN_RGB;
	
	gpio_set_value(vga_info.gpio_sel, !(vga_info.gpio_sel_enable));
	
	rk_fb_switch_screen(&screen, 0 , vga_info.video_source);
	
	printk("firefly vga standby\n");

	return 0;
}

static int firefly_vga_set_enable(struct rk_display_device *device, int enable)
{
	//if(vga_monspecs.suspend)
	//	return 0;
	printk("%s %d\n",__FUNCTION__,__LINE__);
	//if(vga_monspecs.enable != enable || vga_monspecs.mode_set != vga_info.mode)
	//{
		if(enable == 0 && vga_monspecs.enable)
		{
			vga_monspecs.enable = 0;
			firefly_vga_standby(0);
		}
		else if(enable == 1)
		{
			firefly_switch_fb(vga_monspecs.mode, vga_monspecs.mode_set);
			vga_monspecs.enable = 1;
		}
	//}
	return 0;
}

static int firefly_vga_get_enable(struct rk_display_device *device)
{
    printk("%s %d\n",__FUNCTION__,__LINE__);
	return vga_monspecs.enable;
}

static int firefly_vga_get_status(struct rk_display_device *device)
{
    printk("%s %d\n",__FUNCTION__,__LINE__);
	return 1;
}

static int firefly_vga_get_modelist(struct rk_display_device *device, struct list_head **modelist)
{
    printk("%s %d\n",__FUNCTION__,__LINE__);
	*modelist = &(vga_monspecs.modelist);
	return 0;
}

static int firefly_vga_set_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
	int i;
	printk("%s %d\n",__FUNCTION__,__LINE__);

	for(i = 0; i < ARRAY_SIZE(sda7123_vga_mode); i++)
	{
		if(fb_mode_is_equal(&sda7123_vga_mode[i], mode))
		{	
			if( ((i + 1) != vga_info.mode) )
			{
				vga_monspecs.mode_set = i + 1;
				vga_monspecs.mode = (struct fb_videomode *)&sda7123_vga_mode[i];
			}
			return 0;
		}
	}
	
	return -1;
}

static int firefly_vga_get_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
    printk("%s %d\n",__FUNCTION__,__LINE__);
	*mode = *(vga_monspecs.mode);
	return 0;
}

static struct rk_display_ops firefly_vga_display_ops = {
	.setenable = firefly_vga_set_enable,
	.getenable = firefly_vga_get_enable,
	.getstatus = firefly_vga_get_status,
	.getmodelist = firefly_vga_get_modelist,
	.setmode = firefly_vga_set_mode,
	.getmode = firefly_vga_get_mode,
};

static int firefly_display_vga_probe(struct rk_display_device *device, void *devdata)
{
	device->owner = THIS_MODULE;
	strcpy(device->type, "VGA");
	device->name = "firefly_vga";
	device->priority = DISPLAY_PRIORITY_VGA;
	device->property = vga_info.property;
	device->priv_data = devdata;
	device->ops = &firefly_vga_display_ops;
	return 1;
}

static struct rk_display_driver display_firefly_vga = {
	.probe = firefly_display_vga_probe,
};

int firefly_register_display_vga(struct device *parent)
{
	int i;
	
	memset(&vga_monspecs, 0, sizeof(struct sda7123_monspecs));
	INIT_LIST_HEAD(&vga_monspecs.modelist);
	for(i = 0; i < ARRAY_SIZE(sda7123_vga_mode); i++)
		display_add_videomode(&sda7123_vga_mode[i], &vga_monspecs.modelist);

	vga_monspecs.mode = (struct fb_videomode *)&(sda7123_vga_mode[vga_info.mode - 1]);
	vga_monspecs.mode_set = vga_info.mode;

	vga_monspecs.ddev = rk_display_device_register(&display_firefly_vga, parent, NULL);
	vga_info.vga = &vga_monspecs;
	rk_display_device_enable(vga_monspecs.ddev);
	
	return 0;
}
