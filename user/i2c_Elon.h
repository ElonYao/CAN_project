#ifndef _I2C_ELON_H
#define _I2C_ELON_H

#ifdef __cplusplus
extern "C" {
#endif
#include "driverlib.h"
#include "string.h"

#define MATH_RAW2EDG 0.087890625f
typedef enum
{
    SUCCESS,
    ERROR_BUS_BUSY,
    ERROR_STOP_NOT_READY,
    INACTIVE,
    SEND_WITH_STOP,
    WRITE_BUSY,
    SEND_WITHOUT_STOP,
    SEND_WITHOUT_STOP_BUSY,
    RESTART,
    READ_BUSY
}status_t;
typedef enum{
    CW=0,
    CCW
}spinDir_t;

typedef struct _AS5600_
{
    uint16_t addSensor;
    uint16_t addReg;
    uint16_t byteNum;
    uint16_t raw[2];
    uint16_t zeroPosition;
    float32_t angleRad;
    float32_t angleDeg;
    spinDir_t dir;
    status_t I2C_status;
    uint16_t retryCounter;
    uint16_t flag_dataRdy;
}as5600Obj;

typedef struct _AS5600_ *as5600Handle;

//Prototypes
as5600Handle encoderInit(void * pmemory,const size_t numBytes );
uint16_t readData(as5600Handle handler);
void HAL_i2cRead(as5600Handle handler);
float32_t calDeg(as5600Handle handler);
void zeroCalibration(as5600Handle handler);
float32_t mechanicalAngleDeg(as5600Handle handler);

#ifdef __cplusplus
}
#endif

#endif
