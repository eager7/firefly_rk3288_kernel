#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>

#include <linux/version.h>   
#include <linux/proc_fs.h>   

#include <linux/fb.h>
#include <linux/rk_fb.h>
#include <linux/display-sys.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <dt-bindings/rkfb/rk_fb.h>
#endif


struct firefly_led_info {
	struct platform_device	*pdev;
	int     power_gpio;
	int     power_enable_value;
	int     work_gpio;
	int     work_enable_value;
};

struct firefly_led_info led_info;

#define POWER_LED 0
#define WORK_LED 1

void firefly_leds_ctrl_ex(int led_num, int value)
{
    switch(led_num) {
        case POWER_LED:
            gpio_direction_output(led_info.power_gpio, (value==1)?led_info.power_enable_value:!(led_info.power_enable_value));
            break;
        case WORK_LED:
            gpio_direction_output(led_info.work_gpio, (value==1)?led_info.work_enable_value:!(led_info.work_enable_value));
            break;
        default:
            break; 
    }
} 

void firefly_leds_ctrl(char *cmd) 
{
    if(strcmp(cmd,"power on") == 0) {
        gpio_direction_output(led_info.power_gpio, led_info.power_enable_value);
    } else if(strcmp(cmd,"power off") == 0) {
        gpio_direction_output(led_info.power_gpio, !(led_info.power_enable_value));
    } else if(strcmp(cmd,"work on") == 0) {
        gpio_direction_output(led_info.work_gpio, led_info.work_enable_value);
    } else if(strcmp(cmd,"work off") == 0) {
        gpio_direction_output(led_info.work_gpio, !(led_info.work_enable_value));
    }
}

#define USER_PATH "driver/firefly-leds" 
static struct proc_dir_entry *firefly_led_entry; 

#define INFO_PROC "LED Control: power/work on/off"

int proc_read_information(char *page, char **start, off_t off, int count, int *eof, void *data) 
{
    return 0;
}

int proc_write_information(struct file *file, const char *buffer, unsigned long count, void *data) 
{ 
    #define cmd_len 16
    char cmd[cmd_len];
    int len = count,i;
    
    memset(cmd,0,cmd_len);
    if(len >= cmd_len) len = cmd_len - 1;
    memcpy(cmd,buffer,len);
    
    for(i = 0; i < len; i++) {  // 过滤非ASCII符号
        if(cmd[i] < 0x20 || cmd[i] >= 0x7F)  cmd[i] = 0;
    }

    firefly_leds_ctrl(cmd);
    
    return count;
} 

static const struct file_operations firefly_led_proc_fops = {
	.owner		= THIS_MODULE, 
	.write		= proc_write_information,
    .read		= proc_read_information,  
}; 


static int firefly_led_probe(struct platform_device *pdev)
{
    int ret = -1;
    int gpio, rc,flag;
	struct device_node *led_node = pdev->dev.of_node;

    led_info.pdev = pdev;

	gpio = of_get_named_gpio_flags(led_node,"led-power", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-power: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "led_power");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.power_gpio = gpio;
	led_info.power_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(led_info.power_gpio, led_info.power_enable_value);
	
	gpio = of_get_named_gpio_flags(led_node,"led-work", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-power: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "led_power");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.work_gpio = gpio;
	led_info.work_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	gpio_direction_output(led_info.work_gpio, !(led_info.work_enable_value));

    // Create a test entry under USER_ROOT_DIR   
    firefly_led_entry = proc_create(USER_PATH, 0666, NULL, &firefly_led_proc_fops);   
    if (NULL == firefly_led_entry)   
    {   
        goto failed_1;   
    } 
    	
	printk("%s %d\n",__FUNCTION__,__LINE__);
	
	return 0;  //return Ok

failed_1:
	return ret;
}


static int firefly_led_remove(struct platform_device *pdev)
{ 
    remove_proc_entry(USER_PATH, NULL); 
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_rk_firefly_led_match[] = {
	{ .compatible = "firefly,led" },
	{ /* Sentinel */ }
};
#endif

static struct platform_driver firefly_led_driver = {
	.probe		= firefly_led_probe,
	.remove		= firefly_led_remove,
	.driver		= {
		.name	= "firefly-led",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_rk_firefly_led_match,
#endif
	},

};


static int __init firefly_led_init(void)
{
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
    return platform_driver_register(&firefly_led_driver);
}

static void __exit firefly_led_exit(void)
{
	platform_driver_unregister(&firefly_led_driver);
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
}

subsys_initcall(firefly_led_init);
module_exit(firefly_led_exit);

MODULE_AUTHOR("liaohm <teefirefly@gmail.com>");
MODULE_DESCRIPTION("Firefly LED driver");
MODULE_LICENSE("GPL");
