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

struct firefly_vga_info {
	struct platform_device		*pdev;	
	int						video_source;
	int						property;
	int						mode;
	struct sda7123_monspecs *vga;
	int     gpio_sel;
	int     gpio_sel_enable;
	int     gpio_pwn;
	int     gpio_pwn_enable;
};

extern struct firefly_vga_info vga_info;

//int firefly_register_display_vga(struct device *parent);


#endif
