#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>


#define SYNOP_DEVICE "Synaptics RMI4 (Minimal)"

#define TPD_FUNC_ENTER printk("%s: (ENTER) %s\n", SYNOP_DEVICE, __func__)
#define TPD_FUNC_LEAVE printk("%s: (LEAVE) %s\n", SYNOP_DEVICE, __func__)

#define SYNOP_ERROR(msg, args...)  pr_err("%s: [ERROR] %s: " msg, SYNOP_DEVICE, __func__, ##args)
#define SYNOP_DEBUG(msg, args...) pr_info("%s: [DEBUG] %s: " msg, SYNOP_DEVICE, __func__, ##args)


struct synop_touchscreen_data {

    /* Contains touchscreen data. */

    // Client.
    struct i2c_client * client;

    // Input.
    struct input_dev * input_dev;
    uint32_t max_x;
    uint32_t max_y;

    // Mutex.
    struct mutex mutex;
    struct mutex mutexreport;
    spinlock_t lock;

    // Hardware - Regulators.
    struct regulator * vdd;
    struct regulator * vio;

    // IRQ.
    int      irq;
    int      irq_gpio;
    uint32_t irq_flags;
    atomic_t irq_enable;
};

static int F01_CMND;
static int F01_CTRL;
static int F01_DATA;
static int F12_DATA;

static int F01_PRODUCT; // Register Address of Product ID
static int F12_2DINPUT; // Register Address of 2D Input

static struct synop_touchscreen_data * SYNOP = NULL;


#include "synaptics_oneplus_i2c.h"
#include "synaptics_oneplus_poll.h"
#include "synaptics_oneplus_cmnd.h"
#include "synaptics_oneplus_read.h"
#include "synaptics_oneplus_init.h"


static int synop_touchscreen_probe(struct i2c_client * client)
{

    /* Sets up everything for use. */

    int ret = 0;
    char product_id[30];

//    TPD_FUNC_ENTER;

    SYNOP = kzalloc(sizeof(struct synop_touchscreen_data), GFP_KERNEL);
    if (SYNOP == NULL) {
        kfree(SYNOP);

//        TPD_FUNC_LEAVE;
        return -ENOMEM;
    }

    SYNOP -> client = client;
    i2c_set_clientdata(client, SYNOP);

    synop_read_dts_properties(&client -> dev);

    // Initialize the power
    ret = synop_cmnd_power_on();
    if (ret < 0)
        SYNOP_ERROR("Function `synop_cmnd_power_on` failed!\n");

    msleep(100);

    mutex_init(&SYNOP -> mutex);
    mutex_init(&SYNOP -> mutexreport);
    atomic_set(&SYNOP -> irq_enable, 0);

    spin_lock_init(&SYNOP -> lock);

    ret = synop_read_register_addresses();
    if (ret < 0)
        SYNOP_ERROR("Function `synop_read_register_addresses` failed!\n");

    // Display the name of the device.
    synop_i2c_read_block(F01_PRODUCT, sizeof(product_id), product_id);
//    SYNOP_DEBUG("Device Product ID: %s\n", product_id);

    ret = synop_cmnd_reset();
    if (ret < 0)
        SYNOP_ERROR("Function `synop_cmnd_reset` failed!\n");

    ret = synop_init_input(product_id);
    if (ret < 0)
        SYNOP_ERROR("Function `synop_init_input` failed!\n");

    ret = synop_init_irq();
    if (ret < 0)
        SYNOP_ERROR("Function `synop_init_irq` failed!\n");

//    TPD_FUNC_LEAVE;
    return 0;
}


static void synop_touchscreen_remove(struct i2c_client * client)
{
    /* Cleans up everything. Called when module is removed. */

//    TPD_FUNC_ENTER;

    input_unregister_device(SYNOP -> input_dev);
    kfree(SYNOP);

    synop_poll_disable();
    gpio_direction_input(SYNOP -> irq_gpio);
    gpio_set_value(SYNOP -> irq_gpio, 0);
    gpio_free(SYNOP -> irq_gpio);

    synop_cmnd_power_off();

//    TPD_FUNC_LEAVE;
}


static const struct i2c_device_id synaptics_touchscreen_id[] = {
    {
        SYNOP_DEVICE, 0
    },
    {}
};


static struct of_device_id synop_touchscreen_compatible[] = {
    {
        .compatible = "syna,rmi4-i2c"
    },
    {},
};


static const struct dev_pm_ops synop_touchscreen_pm = {
    .suspend = NULL,
    .resume  = NULL
};


static struct i2c_driver synop_touchscreen_driver = {
    .probe  = synop_touchscreen_probe,
    .remove = synop_touchscreen_remove,

    .id_table = synaptics_touchscreen_id,

    .driver = {
        .name           = SYNOP_DEVICE,
        .of_match_table = synop_touchscreen_compatible,
        .pm             = &synop_touchscreen_pm,
    },
};


module_i2c_driver(synop_touchscreen_driver);

MODULE_DESCRIPTION("Synaptics S3203 Touchscreen Driver");
MODULE_LICENSE("GPL");
