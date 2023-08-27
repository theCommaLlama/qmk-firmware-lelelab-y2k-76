#include "aht_sensor.h"
#include "quantum.h"
#include "i2c_master.h"

#define AHT21_ADDR 0x38


bool trig_aht21(void) {
    const unsigned char trig[] = {0xac, 0x33, 0x00};
    return (i2c_transmit(AHT21_ADDR << 1, &trig[0], 3, 100) == I2C_STATUS_SUCCESS);
}

bool read_aht21(sensor_data_t *dout) {
    unsigned char buf[8];
    if (i2c_receive(AHT21_ADDR << 1, buf, 7, 100) != I2C_STATUS_SUCCESS) {
        print("eRd\n");
        return false;
    }
    if (buf[0] & 0x80) {
        print("eBusy\n");
        return false;
    }

    // the sensor gives 20 bits data for H and T, in pattern as follow,
    // HHHHHHHH, HHHHHHHH, HHHHTTTT, TTTTTTTT, TTTTTTTT
    // but we only use 16 bit data each. the least 4 bits are ignored.

    uint16_t a = buf[1];
    a <<= 8;
    a |= buf[2];

    float f = a;
    f /= 0x10000;
    f *= 100;
    dout->h_int = (int)f;
    dout->h_dec = (int)((f - (int)f) * 10);



    a = (buf[3] & 0x0f);
    a <<= 8;
    a |= buf[4];
    a <<= 4;
    a |= (0x0f & (buf[5]>>4));

    f = a;
    f /= 0x10000;
    f = f*200 - 50;
    dout->t_int = (int)f;
    dout->t_dec = (int)((f - (int)f) * 10);

    //Convert to Fahrenheit
    float f_convert = (f*(9/5))+32;

    dout->t_f_int = (int)(f_convert);
    dout->t_f_dec = (int)((f_convert - (int)f_convert) * 10);

    return true;
}
