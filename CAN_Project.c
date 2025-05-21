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
#include "c2000_freertos.h"
#include "FreeRTOS.h"


//Function prototypes
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationMallocFailedHook( void );
void rtosInit(void);

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

struct canBroadData
{
    float32_t angleDeg;
    float32_t angularSpeed;
};
QueueHandle_t gCANPhysiqueue;

static uint8_t gflQueueBuf[1*sizeof(struct canBroadData)];
static StaticQueue_t gQueueStaticStruct;

//
// Main
//
void main(void)
{

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
    rtosInit();
    while(1)
    {

    }
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

//AS5600 data reading task
void as5600Task(void *pvParameters)
{
    const TickType_t xDelay = 2 / portTICK_PERIOD_MS;
    struct canBroadData data;
    for(;;)
    {
        while(!currentSensor->flag_dataRdy)
        {
            HAL_i2cRead(currentSensor);
        }
        mechanicalAngleDeg(currentSensor);
        //Angular speed observer
        speedObserverRun(AOHandler,currentSensor->angleRad);
        data.angleDeg=currentSensor->angleDeg;
        data.angularSpeed=AOHandler->out;
        xQueueOverwrite(gCANPhysiqueue,&data);
        currentSensor->flag_dataRdy=0;
        vTaskDelay(xDelay);
    }

}

void canMessageTask(void * pvParameters)
{
    canMsg_t *obj=(canMsg_t *)pvParameters;
    struct canBroadData data;
    uint16_t temp;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = obj->tRate;
    xLastWakeTime=xTaskGetTickCount();
    for(;;)
    {
        xTaskDelayUntil(&xLastWakeTime, xFrequency);
        xQueueReceive(gCANPhysiqueue, &data, 0);
        //angle data packaging
        if(data.angleDeg>=0)
        {
            temp= data.angleDeg/ANGLEFACTOR;
        }
        else
        {
            temp=~((uint16_t)((0-data.angleDeg)/ANGLEFACTOR))+1;
        }
        obj->data[0]=temp&0xFF;
        obj->data[1]=(temp>>8)&0xFF;
        temp=(data.angularSpeed*RADS2RPM+10000U)*RPMFACTOR;
        obj->data[2]=temp&0xFF;
        obj->data[3]=(temp>>8)&0xFF;
        HAL_sendCAN(obj);
    }
}
void rtosInit(void)
{   //create queue
    gCANPhysiqueue=xQueueCreateStatic(1,sizeof(struct canBroadData),gflQueueBuf,&gQueueStaticStruct);
    //create tasks
    xTaskCreate(as5600Task, "encoderTask", 128, NULL, 2, NULL);
    xTaskCreate(canMessageTask, "CANtask", 128, &angle, 1, NULL);
    vTaskStartScheduler();

}
//
// vApplicationStackOverflowHook - Checks run time stack overflow
//
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

//
// vApplicationMallocFailedHook - Hook function for catching pvPortMalloc() failures
//
void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

//
// End of File
//
