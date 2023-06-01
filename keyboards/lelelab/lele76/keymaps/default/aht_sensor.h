#pragma once
#include "quantum.h"

typedef struct {
    uint8_t h_int;
    uint8_t h_dec;
    uint8_t t_int;
    uint8_t t_dec;
} sensor_data_t;

bool trig_aht21(void);
bool read_aht21(sensor_data_t* dout);

float get_humidity_float(const sensor_data_t* din);
float get_temperature_float(const sensor_data_t* din);
