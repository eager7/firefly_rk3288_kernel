#ifndef _FIREFLY_VGA_H
#define _FIREFLY_VGA_H

#include <linux/fb.h>
#include <linux/rk_fb.h>
#include <linux/display-sys.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <dt-bindings/rkfb/rk_fb.h>
#endif

#define GPIO_HIGH 1
#define GPIO_LOW 0

struct sda7123_monspecs {
	struct rk_display_device	*ddev;
	unsigned int				enable;
	unsigned int				suspend;
	struct fb_videomode			*mode;
	struct list_head			modelist;
	unsigned int 				mode_set;
};

struct vga_ddc_dev
{
	unsigned char        *edid;
	struct i2c_client    *client;
	struct fb_monspecs   specs;
	struct list_head     modelist;        //monitor mode list
	int    modelen;                       //monitor mode list len
//	const struct fb_videomode  *current_mode;     //current mode
	int						video_source;
	int						property;
	int						modeNum;
	struct sda7123_monspecs *vga;
	int     gpio_sel;
	int     gpio_sel_enable;
	int     gpio_pwn;
	int     gpio_pwn_enable;
	int  ddc_check_ok;
	int ddc_timer_start;
	int first_start;
	int set_mode;
};

extern struct vga_ddc_dev *ddev;
extern struct timer_list timer_vga_ddc;

#define VGA_SOURCE_EXTERN 1
#define VGA_SOURCE_INTERNAL 0
extern void vga_switch_source(int source);

//int firefly_register_display_vga(struct device *parent);


#endif
