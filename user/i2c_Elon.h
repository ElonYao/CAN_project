#ifndef _I2C_ELON_H
#define _I2C_ELON_H

#ifdef __cplusplus
extern "C" {
#endif
#include "driverlib.h"
#include "string.h"

typedef enum{
    CW=0,
    CCW
}spinDir_t;

typedef struct _AS5600_
{
    uint16_t addSensor;
    uint16_t addReg;
    uint16_t raw;
    float32_t angleRad;
    float32_t angleDeg;
    float32_t zeroPosition;
    spinDir_t dir;
}as5600_t;

typedef struct _AS5600_ *as5600Handler;

//Prototypes
as5600Handler encoderInit(void * pmemory,const size_t numBytes );

#ifdef __cplusplus
}
#endif

#endif
