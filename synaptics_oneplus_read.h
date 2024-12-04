/**
 * File: synaptics_oneplus_read.h
 *
 */


static int synop_read_dts_properties(struct device * dev)
{
    /* Reads the Device Tree Structure for device-specific properties. */

    int ret = 0;

//    TPD_FUNC_ENTER;

    ret = ret || of_property_read_u32(of_get_child_by_name(dev->of_node, "rmi4-f12"), "syna,clip-x-high", &SYNOP -> max_x);
    if (ret)
        SYNOP_ERROR("The touchscreen width is not specified (Returned %d)\n", ret);

    ret = ret || of_property_read_u32(of_get_child_by_name(dev->of_node, "rmi4-f12"), "syna,clip-y-high", &SYNOP -> max_y);
    if (ret)
        SYNOP_ERROR("The touchscreen height is not specified (Returned %d)\n", ret);

    SYNOP -> irq_gpio = of_get_named_gpio(dev -> of_node, "interrupts-extended", 0);

    SYNOP -> vdd = regulator_get(&SYNOP -> client -> dev, "vdd");
    SYNOP -> vio = regulator_get(&SYNOP -> client -> dev, "vio");

    // Validating the IRQ GPIO.
    if(SYNOP -> irq_gpio < 0) {
        ret = ret || SYNOP -> irq_gpio;
        SYNOP_ERROR("The IRQ GPIO is not specified (Returned %d)\n", ret);
    }

    // Validating the vdd regulator.
    if (IS_ERR(SYNOP -> vdd)) {
        ret = ret || PTR_ERR(SYNOP -> vdd);
        SYNOP_ERROR("Could not get regulator `vdd` (Returned %d)\n", ret);
    }

    // Validating the vio regulator.
    if (IS_ERR(SYNOP -> vio)) {
        ret = ret || PTR_ERR(SYNOP -> vio);
        SYNOP_ERROR("Could not get regulator `vio` (Returned %d)\n", ret);
    }

//    TPD_FUNC_LEAVE;
    return ret;
}


static int synop_read_register_addresses(void)
{
    /* Reads and stores the important register addresses. */

    int ret = 0;
    uint8_t buf[4];

//    TPD_FUNC_ENTER;

    memset(buf, 0, sizeof(buf));

    // Setting the page to 0.
    ret = ret || synop_i2c_write_byte(0xFF, 0x0);

    ret = ret || synop_i2c_read_block(0xDD, 4, &(buf[0]));

    F12_DATA    = buf[3];
    F12_2DINPUT = 0x0B;

    ret = ret || synop_i2c_read_block(0xE3, 4, &(buf[0]));

    F01_CMND    = buf[1];
    F01_CTRL    = buf[2];
    F01_DATA    = buf[3];
    F01_PRODUCT = buf[0] + 11;

//    TPD_FUNC_LEAVE;
    return ret;
}
