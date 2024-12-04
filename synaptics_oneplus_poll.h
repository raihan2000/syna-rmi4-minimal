/**
 * File: synaptics_oneplus_poll.h
 *
 */


struct synop_poll_point {

    /* Stores information about an input slot. */

    int x;
    int y;
    int z;
    int raw_x;
    int raw_y;
    unsigned char status;
};


static void synop_poll_enable(void)
{
    /* Enables the IRQ, which enables touch polling. */

//    TPD_FUNC_ENTER;

    spin_lock(&SYNOP -> lock);

    if (atomic_read(&SYNOP -> irq_enable) == 0) {
        if (SYNOP -> irq)
            enable_irq(SYNOP -> irq);

        atomic_set(&SYNOP -> irq_enable, 1);
    }

    spin_unlock(&SYNOP -> lock);
}


static void synop_poll_disable(void)
{
    /* Disables the IRQ, which disables touch polling. */

//    TPD_FUNC_ENTER;

    spin_lock(&SYNOP -> lock);

    if (atomic_read(&SYNOP -> irq_enable) == 1) {
        if (SYNOP -> irq)
            disable_irq_nosync(SYNOP -> irq);

        atomic_set(&SYNOP -> irq_enable, 0);
    }

    spin_unlock(&SYNOP -> lock);
}


void synop_poll_touch(void)
{
    /* Handles touch interrupts. */

    int i = 0;

    uint8_t  object_attention[2];
    uint8_t  count_data    = 0;
    uint16_t total_status  = 0;

    uint8_t buf[90];
    struct  synop_poll_point point;

    uint32_t finger_info   = 0;
    uint8_t  finger_status = 0;
    uint8_t  finger_num    = 0;

//    TPD_FUNC_ENTER;

    memset(buf, 0, sizeof(buf));

    mutex_lock(&SYNOP -> mutexreport);

    synop_i2c_read_block(F12_2DINPUT, 2, object_attention);

    total_status = (object_attention[1] << 8) | object_attention[0];

    if (total_status) {
        while (total_status) {
            count_data ++;
            total_status >>= 1;
        }
    }
    else
        count_data = 0;

    if (count_data > 10) {
        SYNOP_ERROR("%d fingers have been reported, this is not possible.\n", count_data);
        mutex_unlock(&SYNOP -> mutexreport);

//        TPD_FUNC_LEAVE;
        return;
    }

    synop_i2c_read_block(F12_DATA, count_data * 8 + 1, buf);

    for (i = 0; i < count_data; i ++) {
        point.x = ((buf[i * 8 + 2] & 0x0f) << 8) | (buf[i * 8 + 1] & 0xff);
        point.y = ((buf[i * 8 + 4] & 0x0f) << 8) | (buf[i * 8 + 3] & 0xff);
        point.z = buf[i * 8 + 5];

        point.raw_x = buf[i * 8 + 6] & 0x0f;
        point.raw_y = buf[i * 8 + 7] & 0x0f;

        point.status = buf[i * 8];

        finger_info <<= 1;
        finger_status = point.status & 0x03;

        if (finger_status) {
            input_mt_slot(SYNOP -> input_dev, i);
            input_mt_report_slot_state(SYNOP -> input_dev, MT_TOOL_FINGER, finger_status);

            input_report_abs(SYNOP -> input_dev, ABS_MT_POSITION_X, point.x);
            input_report_abs(SYNOP -> input_dev, ABS_MT_POSITION_Y, point.y);
            input_report_abs(SYNOP -> input_dev, ABS_MT_PRESSURE,   point.z);

            input_report_abs(SYNOP -> input_dev, ABS_MT_TOUCH_MAJOR, max(point.raw_x, point.raw_y));
            input_report_abs(SYNOP -> input_dev, ABS_MT_TOUCH_MINOR, min(point.raw_x, point.raw_y));

            input_report_key(SYNOP -> input_dev, BTN_TOUCH, 1);

            // These events will be based on the first finger in queue.
            if (i == 0) {
                input_report_abs(SYNOP -> input_dev, ABS_X,        point.x);
                input_report_abs(SYNOP -> input_dev, ABS_Y,        point.y);
                input_report_abs(SYNOP -> input_dev, ABS_PRESSURE, point.z);
            }

            finger_num ++;
            finger_info |= 1;
        }
    }

    finger_info <<= (10 - count_data);

    for (i = 0; i < 10; i ++) {
        finger_status = (finger_info >> (10 - i - 1)) & 1;

        if (!finger_status) {
            input_mt_slot(SYNOP -> input_dev, i);
            input_mt_report_slot_state(SYNOP -> input_dev, MT_TOOL_FINGER, finger_status);
        }
    }

    if (finger_num == 0) {
        input_report_key(SYNOP -> input_dev, BTN_TOUCH, 0);
        input_report_key(SYNOP -> input_dev, ABS_PRESSURE, 0);
    }

    input_sync(SYNOP -> input_dev);

    mutex_unlock(&SYNOP -> mutexreport);

//    TPD_FUNC_LEAVE;
    return;
}


static void synop_poll_main(void)
{
    /* Main function for polling. */

    int ret = 0;

    unsigned char buf[2] = {0};
    uint8_t ds_reg = 0;
    uint8_t is_reg = 0;

    uint8_t tmp_mode = 0;

//    TPD_FUNC_ENTER;

    synop_i2c_read_block(F01_DATA, 2, buf);

    ds_reg = buf[0];
    is_reg = buf[1] & 0x7f;

    if (ds_reg & 0x80) {
        tmp_mode = synop_i2c_read_byte(F01_CTRL);

        tmp_mode = tmp_mode & 0xF8; // Bit 0 - Bit 2 (Mode)
        tmp_mode = tmp_mode | 0x80;
        tmp_mode = tmp_mode & 0xDF; // Clear Bit 6 (Change status)

//        SYNOP_DEBUG("Setting touchscreen mode (Value 0x%x)\n", tmp_mode);

        ret = synop_i2c_write_byte(F01_CTRL, tmp_mode);
        if (ret < 0)
            SYNOP_ERROR("Failed to set touchscreen mode\n");
    }

    if (is_reg & 0x04)
        synop_poll_touch();

//    TPD_FUNC_LEAVE;
    return;
}


static irqreturn_t synop_poll_irq(int irq, void * dev_id)
{
    /* IRQ function for polling. */

//    TPD_FUNC_ENTER;

    synop_poll_disable();
    synop_poll_main();
    synop_poll_enable();

//    TPD_FUNC_LEAVE;
    return IRQ_HANDLED;
}
