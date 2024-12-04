/**
 * File: synaptics_oneplus_cmnd.h
 *
 */


static int synop_cmnd_power_on(void)
{
    /* Enable the regulators which control the power supply. */

    int ret = 0;

//    TPD_FUNC_ENTER;

    if (!IS_ERR(SYNOP -> vdd)) {
        ret = ret || regulator_enable(SYNOP -> vdd);

        if (ret)
            dev_err(&SYNOP -> client -> dev, "Failed to enable regulator `vdd` (Returned %d)\n", ret);
    }

    usleep_range(10 * 1000, 10 * 1000);

    if (!IS_ERR(SYNOP -> vio)) {
        ret = ret || regulator_enable(SYNOP -> vio);

        if (ret)
            dev_err(&SYNOP -> client -> dev, "Failed to enable regulator `vio` (Returned %d)\n", ret);
    }

    usleep_range(10 * 1000, 10 * 1000);

//    TPD_FUNC_LEAVE;
    return ret;
}


static int synop_cmnd_power_off(void)
{
    /* Disable the regulators which control the power supply. */

    int ret = 0;

//    TPD_FUNC_ENTER;

    if (!IS_ERR(SYNOP -> vio)) {
        ret = ret || regulator_disable(SYNOP -> vio);

        if (ret)
            dev_err(&SYNOP -> client -> dev, "Failed to disable regulator `vio` (Returned %d)\n", ret);
    }

    if (!IS_ERR(SYNOP -> vdd)) {
        ret = ret || regulator_disable(SYNOP -> vdd);

        if (ret)
            dev_err(&SYNOP -> client -> dev, "Failed to disable regulator `vdd` (Returned %d)\n", ret);
    }

//    TPD_FUNC_LEAVE;
    return ret;
}


static int synop_cmnd_reset(void)
{
    /* Initiates a reset command to the touch panel. */

    int ret = 0;

//    TPD_FUNC_ENTER;

    synop_poll_disable();

    ret = synop_i2c_write_byte(F01_CMND, 0x01);
    if (ret < 0)
        SYNOP_ERROR("Reset command failed (Returned %d)\n", ret);

    msleep(100);
    synop_poll_enable();

//    TPD_FUNC_LEAVE;
    return ret;
}
