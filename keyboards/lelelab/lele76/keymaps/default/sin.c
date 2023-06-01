#include "sin.h"
#include "quantum.h"
#include "progmem.h"


const uint16_t lut[]  __attribute__ ((aligned(2))) PROGMEM = 
{         
	0,  1144,  2287,  3430,  4571, 5712,  6850,   7987,  9121, 10252,
11380, 12505, 13625, 14742, 15854, 16962, 18064, 19161, 20251, 21336,
22414, 23486, 24550, 25607, 26655, 27696, 28729, 29752, 30767, 31772,
32767, 33753, 34728, 35693, 36647, 37589, 38521, 39440, 40347, 41243,
42125, 42995, 43851, 44695, 45524, 46340, 47142, 47929, 48702, 49460,
50203, 50930, 51642, 52339, 53019, 53683, 54331, 54962, 55577, 56174,
56755, 57318, 57864, 58392, 58902, 59395, 59869, 60325, 60763, 61182,
61583, 61965, 62327, 62671, 62996, 63302, 63588, 63855, 64103, 64331,
64539, 64728, 64897, 65047, 65176, 65286, 65375, 65445, 65495, 65525,
65535
};


float SIN(uint16_t angle) {
	uint32_t temp;
	float ans;
	
	if (angle >= 0 && angle < 91) {
        temp = (uint16_t)pgm_read_word(&lut[angle]);
        ans  = (float)temp;
    }
	else if (angle > 90 && angle < 181) {
        temp = (uint16_t)pgm_read_word(&lut[180 - angle]);
        ans  = (float)temp;
    }
	else if (angle > 180 && angle < 271) {
        temp = (uint16_t)pgm_read_word(&lut[angle - 180]);
        ans  = -(float)temp;
    }
	else {
        temp = (uint16_t)pgm_read_word(&lut[360 - angle]);
        ans  = -(float)temp;
    }

    return ans / 0xffff;
}


float COS(uint16_t angle) {
	uint32_t temp;
	float ans;
	
	if (angle >= 0 && angle < 91) {
        temp = (uint16_t)pgm_read_word(&lut[90-angle]);
        ans  = (float)temp;
	}
	else if (angle > 90 && angle < 181) {
        temp = (uint16_t)pgm_read_word(&lut[angle-90]);
        ans  = -(float)temp;
	}
	else if (angle > 180 && angle < 271) {
        temp = (uint16_t)pgm_read_word(&lut[270-angle]);
        ans  = -(float)temp;
	}
	else {
        temp = (uint16_t)pgm_read_word(&lut[angle-270]);
        ans  = (float)temp;
	}

    return ans / 0xffff;
}

