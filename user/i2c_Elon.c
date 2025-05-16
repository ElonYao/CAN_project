#include "i2c_Elon.h"

as5600Handle encoderInit(void * pmemory,const size_t numBytes )
{
    as5600Handle handler;
    as5600Obj *obj;
    if(numBytes<sizeof(as5600Obj))
    {
        return((as5600Handle)NULL);
    }
    handler=(as5600Handle)pmemory;
    obj=(as5600Obj*)handler;
    obj->addSensor=0x36;
    obj->addReg=0x0C;
    obj->angleDeg=0.0f;
    obj->angleRad=0.0f;
    obj->zeroPosition=0;
    obj->dir=CW;
    obj->byteNum=2;
    obj->retryCounter=0;
    obj->I2C_status=INACTIVE;
    obj->flag_dataRdy=0;
    return handler;
}

uint16_t readData(as5600Handle handler)
{
    as5600Obj *obj=(as5600Obj *)handler;

    //Check if a previous communication is completed
    if(HWREGH(I2CB_BASE+I2C_O_MDR)& I2C_MDR_STP)
    {
        return ERROR_STOP_NOT_READY;
    }

    //set slave address
    HWREGH(I2CB_BASE+I2C_O_SAR)=obj->addSensor;
    if(obj->I2C_status==SEND_WITHOUT_STOP)
    {
        //check the bus
        if(I2C_isBusBusy(I2CB_BASE))
        {
            return ERROR_BUS_BUSY;
        }

        //set number of byte to be transfered
        HWREGH(I2CB_BASE+I2C_O_CNT)=1;
        HWREGH(I2CB_BASE+I2C_O_DXR)=obj->addReg;
        I2C_setConfig(I2CB_BASE, I2C_CONTROLLER_SEND_MODE);
        I2C_sendStartCondition(I2CB_BASE);
    }
    else if(obj->I2C_status==RESTART)
    {
        HWREGH(I2CB_BASE+I2C_O_CNT)=obj->byteNum;
        I2C_setConfig(I2CB_BASE, I2C_CONTROLLER_RECEIVE_MODE);
        I2C_sendStartCondition(I2CB_BASE);
        I2C_sendStopCondition(I2CB_BASE);
    }
    return SUCCESS;
}

void HAL_i2cRead(as5600Handle handler)
{
    as5600Obj *obj=(as5600Obj *)handler;
    if(obj->I2C_status==INACTIVE)
    {
            obj->I2C_status=SEND_WITHOUT_STOP;
            while(readData(handler)!=SUCCESS && obj->retryCounter<=2)
            {
                 obj->retryCounter++;
            }
            obj->retryCounter=0;
            obj->I2C_status=SEND_WITHOUT_STOP_BUSY;
     }
     if(obj->I2C_status==RESTART)
     {
                while(readData(handler)!=SUCCESS && obj->retryCounter<=2)
                {
                    obj->retryCounter++;
                }
                obj->retryCounter=0;
                obj->I2C_status=READ_BUSY;
      }

}

float32_t mechanicalAngleDeg(as5600Handle handler)
{
    as5600Obj *obj=(as5600Obj *)handler;
    float32_t angleDeg=0.0f;
    int16_t delta=(obj->raw[0]<<8)|obj->raw[1];
    delta-=obj->zeroPosition;
    angleDeg=delta*MATH_RAW2EDG;//signed
    angleDeg=(angleDeg>180.0f)?(angleDeg-360.0f):((angleDeg<-180.0f)? (angleDeg+360.0f):angleDeg);
    obj->angleDeg=angleDeg;
    return angleDeg;
}

void zeroCalibration(as5600Handle handler)
{
    as5600Obj *obj=(as5600Obj *)handler;
    while(!obj->flag_dataRdy)
    {
        HAL_i2cRead(handler);
    }
    obj->zeroPosition=(obj->raw[0]<<8)|obj->raw[1];
    obj->flag_dataRdy=0;
}

