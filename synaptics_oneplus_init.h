/**
 * File: synaptics_oneplus_init.h
 *
 */


static int synop_init_input(const char * product_id)
{
    /* Initialize the Linux input device associated with this touchscreen. */

    int ret = 0;
    char * input_dev_name;

//    TPD_FUNC_ENTER;

    SYNOP -> input_dev = input_allocate_device();
    if (SYNOP -> input_dev == NULL) {
        ret = -ENOMEM;
        SYNOP_ERROR("Failed to allocate input device\n");

//        TPD_FUNC_LEAVE;
        return ret;
    }

    input_dev_name = devm_kasprintf(&SYNOP -> client -> dev, GFP_KERNEL, "Synaptics %s", product_id);

    SYNOP -> input_dev -> name = input_dev_name;
    SYNOP -> input_dev -> dev.parent = &SYNOP -> client -> dev;

    set_bit(EV_SYN, SYNOP -> input_dev -> evbit);
    set_bit(EV_ABS, SYNOP -> input_dev -> evbit);
    set_bit(EV_KEY, SYNOP -> input_dev -> evbit);

    set_bit(ABS_MT_TOUCH_MAJOR, SYNOP -> input_dev -> absbit);
    set_bit(ABS_MT_POSITION_X,  SYNOP -> input_dev -> absbit);
    set_bit(ABS_MT_POSITION_Y,  SYNOP -> input_dev -> absbit);
    set_bit(ABS_MT_PRESSURE,    SYNOP -> input_dev -> absbit);

    set_bit(ABS_X,        SYNOP -> input_dev -> absbit);
    set_bit(ABS_Y,        SYNOP -> input_dev -> absbit);
    set_bit(ABS_PRESSURE, SYNOP -> input_dev -> absbit);

    set_bit(INPUT_PROP_DIRECT, SYNOP -> input_dev -> propbit);

    set_bit(BTN_TOUCH, SYNOP -> input_dev -> keybit);

    input_set_abs_params(SYNOP -> input_dev, ABS_MT_TOUCH_MAJOR, 0, 255,                  0, 0);
    input_set_abs_params(SYNOP -> input_dev, ABS_MT_TOUCH_MINOR, 0, 255,                  0, 0);
    input_set_abs_params(SYNOP -> input_dev, ABS_MT_POSITION_X,  0, (SYNOP -> max_x - 1), 0, 0);
    input_set_abs_params(SYNOP -> input_dev, ABS_MT_POSITION_Y,  0, (SYNOP -> max_y - 1), 0, 0);
    input_set_abs_params(SYNOP -> input_dev, ABS_MT_PRESSURE,    0, 0xFF,                 0, 0);

    input_set_abs_params(SYNOP -> input_dev, ABS_X,        0, (SYNOP -> max_x - 1), 0, 0);
	input_set_abs_params(SYNOP -> input_dev, ABS_Y,        0, (SYNOP -> max_y - 1), 0, 0);
    input_set_abs_params(SYNOP -> input_dev, ABS_PRESSURE, 0, 0xFF,                 0, 0);

    input_mt_init_slots(SYNOP -> input_dev, 10, 0);

    input_set_drvdata(SYNOP -> input_dev, SYNOP);

    if (input_register_device(SYNOP -> input_dev)) {
        SYNOP_ERROR("Failed to register input device\n");

        input_unregister_device(SYNOP -> input_dev);

//        TPD_FUNC_LEAVE;
        return -1;
    }

//    TPD_FUNC_LEAVE;
    return ret;
}


static int synop_init_irq(void)
{
    /* Enables the IRQ through the IRQ GPIO. */

    int ret = 0;

//    TPD_FUNC_ENTER;

    if (gpio_is_valid(SYNOP -> irq_gpio)) {
        ret = ret || gpio_request(SYNOP -> irq_gpio, "tp-s3320-irq");

        if (ret)
            SYNOP_ERROR("Unable to request GPIO %d (Returned %d)\n", SYNOP -> irq_gpio, ret);

        ret = gpio_direction_input(SYNOP -> irq_gpio);
        msleep(50);
        SYNOP -> irq = gpio_to_irq(SYNOP -> irq_gpio);
    }

//    SYNOP_DEBUG("IRQ: %d\n", SYNOP -> irq);

    ret = ret || request_threaded_irq(SYNOP -> irq, NULL, synop_poll_irq, SYNOP -> irq_flags | IRQF_ONESHOT, SYNOP_DEVICE, SYNOP);

    if (ret < 0)
        SYNOP_ERROR("Failed to request an IRQ (Returned %d)\n", ret);

//    TPD_FUNC_LEAVE;
    return ret;
}
