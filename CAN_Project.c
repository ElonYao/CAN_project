//#############################################################################
//
// FILE:   CAN_communication.c
//
//! Author: Elon Yao
//! Time: 20250513
//! Description: This is a CAN communication test project
//!
//!
//
//#############################################################################
//
//
// $Copyright:
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"
#include "comm_Elon.h"
#include "i2c_Elon.h"
#include "speedObserver_Elon.h"
#include "circularBuffer.h"

canMsg_t angle={
    .canId=0x123,
    .objID=1,
    .len=8,
    .tRate=10,
    .tRcounter=0,
    .errorCounter=0,
    .msgIDtype=CAN_MSG_FRAME_STD,
    .msgObjtype=CAN_MSG_OBJ_TYPE_TX
};
canMsg_t responseMsg={
    .canId=0x7e8,
    .objID=2,
    .len=8,
    .tRate=0,
    .tRcounter=0,
    .errorCounter=0,
    .msgIDtype=CAN_MSG_FRAME_STD,
    .msgObjtype=CAN_MSG_OBJ_TYPE_TX
};
as5600Obj sensor1;
as5600Handle sensor1Handler;

as5600Obj *currentSensor=&sensor1;

speedObserver_obj angularObserver;
speedObserverHandle AOHandler;

//
// Main
//
void main(void)
{
    uint16_t temp;
    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull-ups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // PinMux and Peripheral Initialization
    //
    Board_init();
    I2C_enableFIFO(I2CB_BASE);
    I2C_clearInterruptStatus(I2CB_BASE, I2C_INT_RXFF | I2C_INT_TXFF);

    //
    // C2000Ware Library initialization
    //
    C2000Ware_libraries_init();
    //Instance initialization
    sensor1Handler=encoderInit(&sensor1,sizeof(sensor1));
    AOHandler=speedObserverInit(&angularObserver,sizeof(angularObserver));
    //
    // Enable Global Interrupt (INTM) and real time interrupt (DBGM)
    //
    EINT;
    ERTM;
    zeroCalibration(currentSensor);
    while(1)
    {
        if(currentSensor->updateCounter>=2)
        {
            while(!currentSensor->flag_dataRdy)
            {
                HAL_i2cRead(currentSensor);
            }
            mechanicalAngleDeg(currentSensor);
            //Angular speed observer
            speedObserverRun(AOHandler,currentSensor->angleRad);
            currentSensor->flag_dataRdy=0;
            currentSensor->updateCounter=0;
        }
        //angle data packaging
        if(currentSensor->angleDeg>=0)
        {
            temp= currentSensor->angleDeg/ANGLEFACTOR;
        }
        else
        {
            temp=~((uint16_t)(-currentSensor->angleDeg/ANGLEFACTOR))+1;
        }
        angle.data[0]=temp&0xFF;
        angle.data[1]=(temp>>8)&0xFF;
        temp=(angularObserver.out*RADS2RPM+10000U)*RPMFACTOR;
        angle.data[2]=temp&0xFF;
        angle.data[3]=(temp>>8)&0xFF;
        HAL_sendCAN(&angle);

    }
}
__interrupt void INT_mainTimer_ISR()
{
    angle.tRcounter++;
    currentSensor->updateCounter++;
    Interrupt_clearACKGroup(INT_mainTimer_INTERRUPT_ACK_GROUP);
}
__interrupt void INT_encoder_I2C_ISR()
{
    I2C_InterruptSource intSource;
    uint16_t index;
    intSource=I2C_getInterruptSource(I2CB_BASE);

    if(intSource==I2C_INTSRC_STOP_CONDITION)
    {
        if(currentSensor->I2C_status==WRITE_BUSY)
        {
            currentSensor->I2C_status=INACTIVE;
        }
        else
        {
            if(currentSensor->I2C_status==SEND_WITHOUT_STOP_BUSY)
            {
                currentSensor->I2C_status=INACTIVE;//retry, may need to limit the numbers
            }
            else if(currentSensor->I2C_status==READ_BUSY)
            {
                currentSensor->I2C_status=INACTIVE;
                for(index=0;index<currentSensor->byteNum;index++)
                {
                    currentSensor->raw[index]=I2C_getData(I2CB_BASE);
                }

                currentSensor->flag_dataRdy=1;

            }
        }
    }
    else if(intSource==I2C_INTSRC_REG_ACCESS_RDY)
    {
        if(I2C_getStatus(I2CB_BASE)& I2C_STS_NO_ACK)
        {
            I2C_sendStopCondition(I2CB_BASE);
            I2C_clearStatus(I2CB_BASE, I2C_STS_NO_ACK);
        }
        else if(currentSensor->I2C_status==SEND_WITHOUT_STOP_BUSY)
        {
            currentSensor->I2C_status=RESTART;
        }
    }
    Interrupt_clearACKGroup(INT_encoder_I2C_INTERRUPT_ACK_GROUP);
}
//
// End of File
//
