/**
 * File: synaptics_oneplus_i2c.h
 *
 */


int synop_i2c_read_block(unsigned char addr, unsigned short length, unsigned char * data)
{
    /* Reads from a block of registers over i2c. */

    int ret;
    unsigned char reg_addr;

    struct i2c_msg msg[] = {
        {
            .addr  = SYNOP -> client -> addr,
            .flags = 0,
            .len   = 1,
            .buf   = &reg_addr,
        },
        {
            .addr  = SYNOP -> client -> addr,
            .flags = I2C_M_RD,
            .len   = length,
            .buf   = data,
        },
    };

    reg_addr = addr & 0xFF;

    ret = i2c_transfer(SYNOP -> client -> adapter, msg, 2) != 2;

    if (ret) SYNOP_ERROR("Failed to read %u bytes from 0x%x", length, reg_addr);

//    SYNOP_DEBUG("Read %d bytes at %#06x: (%*ph)\n", length, addr, length, data);

    return ret;
}


int synop_i2c_write_block(unsigned char addr, unsigned short length, unsigned char const * data)
{

    /* Writes to a block of registers over i2c. */

    int ret;
    unsigned char * buf = kzalloc(length + 1, GFP_KERNEL);

    struct i2c_msg msg[] = {
        {
            .addr  = SYNOP -> client -> addr,
            .flags = 0,
            .len   = length + 1,
            .buf   = buf,
        }
    };

    buf[0] = addr & 0xFF;
    memcpy(&buf[1], &data[0], length);

    ret = i2c_transfer(SYNOP -> client -> adapter, msg, 1) != 1;

    if (ret) SYNOP_ERROR("Failed to write %u bytes from 0x%x", length, buf[0]);

//    SYNOP_DEBUG("Write %d bytes at %#06x: (%*ph)\n", length, addr, length, data);

    kfree(buf);

    return ret;
}


static int synop_i2c_read_byte(unsigned char addr)
{
    /* Reads from a register over i2c. */

    int data = 0;
    unsigned char buf[2] = {0};

    data = synop_i2c_read_block(addr, 1, buf);
    data = buf[0] & 0xFF;

    return data;
}


static int synop_i2c_write_byte(unsigned char addr, unsigned char data)
{
    /* Write to a register over i2c. */

    int ret;
    unsigned char data_send = data;

    ret = synop_i2c_write_block(addr, 1, &data_send);

    return ret;
}
