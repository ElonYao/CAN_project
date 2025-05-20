#ifndef _COMM_ELON_H
#define _COMM_ELON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driverlib.h"
#include "board.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define ANGLEFACTOR 0.005496f
#define RPMFACTOR 3.27675f
typedef struct _canCom_
{
    uint32_t canId;
    uint16_t len;
    uint16_t data[8];
    uint16_t tRate;
    uint16_t tRcounter;
    uint32_t errorCounter;
    uint32_t objID;
    CAN_MsgFrameType msgIDtype;
    CAN_MsgObjType msgObjtype;
}canMsg_t;

typedef struct _cmdSerial_
{
    uint16_t rawCMD[8];
    uint16_t cmdName[3];
    uint16_t cmdPara[5];
    float32_t cmdValue;
    uint16_t flagNewcmd;

}serialCMD;

typedef struct _canCom_ *canHandle;
typedef struct _cmdSerial_ *cmdHandle;

//CAN communication APIs
canHandle canInit(void *memory,const size_t memorySize);
uint16_t messageValidation(uint16_t *buff,uint16_t messageCounter,uint32_t messageID);
void updateCAN(canMsg_t *msg);
void HAL_sendCAN(canMsg_t *msg);

//UsartB communication for HC-05 and PIC18F45K22
cmdHandle cmdInit(void *memory,const size_t memorySize);
void cmdParse(cmdHandle handle);
void comDispatch(cmdHandle handle);
uint16_t usartChecksum(uint16_t *input);



#ifdef __cplusplus
}
#endif

#endif
