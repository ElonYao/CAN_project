#include "comm_Elon.h"

canHandle canInit(void *memory,const size_t memorySize)
{
    canHandle handle;
    canMsg_t *obj;
    if(memorySize < sizeof(canMsg_t))
    {
        return (canHandle)NULL;
    }
    handle = (canHandle)memory;
    obj = (canMsg_t *)handle;
    return handle;
}
uint16_t messageValidation(uint16_t *buff,uint16_t messageCounter,uint32_t messageID)
{
    uint16_t index,checkSum=0;
    for(index=0;index<7;index++)
    {
        checkSum+=buff[index];
    }
    checkSum+=(messageID & 0x0F)+(messageID>>8 & 0x0F)+(messageID>>16 & 0x0F)+(messageID>>24 & 0x0F)+(messageCounter & 0x0F);
    checkSum = ((checkSum >> 4) + checkSum) & 0x0F;

    return checkSum;
}
void updateCAN(canMsg_t *msg)
{
    //signal packaging
    uint32_t temp;
    if(msg->tRcounter>=msg->tRate)
    {
        msg->data[0]=0x02;
        msg->data[1]=0x09;
        msg->data[2]=0x02;
        memset(&msg->data[3],0,5);
        CAN_setupMessageObject(CAN0_BASE, msg->objID, msg->canId, msg->msgIDtype,msg->msgObjtype, 0, 0, msg->len);
        CAN_sendMessage(CAN0_BASE, msg->objID, msg->len,msg->data);
        CAN_getErrorCount(CAN0_BASE, &temp, &msg->errorCounter);
        msg->tRcounter=0;
    }

}
void HAL_sendCAN(canMsg_t *msg)
{

    uint32_t temp;
    CAN_setupMessageObject(CAN0_BASE, msg->objID, msg->canId, msg->msgIDtype,msg->msgObjtype, 0, 0, msg->len);
    CAN_sendMessage(CAN0_BASE, msg->objID, msg->len,msg->data);
    CAN_getErrorCount(CAN0_BASE, &temp, &msg->errorCounter);

}
cmdHandle cmdInit(void *memory,const size_t memorySize)
{
    cmdHandle handle;
    serialCMD *obj;
    if(memorySize < sizeof(serialCMD))
    {
        return (cmdHandle)NULL;
    }
    handle = (cmdHandle)memory;
    obj = (serialCMD *)handle;
    obj->flagNewcmd=0;
    obj->cmdValue=0.0f;
    memset(obj->cmdPara,0,sizeof(obj->cmdPara));
    memset(obj->rawCMD,0,sizeof(obj->rawCMD));
    memset(obj->cmdName,0,sizeof(obj->cmdName));
    return handle;
}

uint16_t usartChecksum(uint16_t *input)
{
    uint16_t checksum=0,index=0;
    //only confirm the first 6 elements
    for(index=0;index<6;index++)
    {
        checksum^=input[index];
    }
    checksum &=0x00FF;
    return checksum;
}

void cmdParse(cmdHandle handle)
{
    serialCMD *obj= (serialCMD *)handle;
    uint16_t dataIndex,n;
    if(obj->flagNewcmd==1 && obj->rawCMD[6]==usartChecksum(obj->rawCMD))
    {
        for(dataIndex=0;obj->rawCMD[dataIndex]!=' ';dataIndex++)
        {
          if(isalpha(obj->rawCMD[dataIndex]))
          {
              obj->cmdName[dataIndex]=toupper(obj->rawCMD[dataIndex]);
          }
          else
          {
              obj->flagNewcmd=0;
              return;
          }
        }
        //byte 3~6
        for(dataIndex=dataIndex+1,n=0;dataIndex<6;dataIndex++,n++)
        {
          if(isdigit(obj->rawCMD[dataIndex]))
          {
            obj->cmdPara[n]=obj->rawCMD[dataIndex];
          }
          else
          {
            obj->flagNewcmd=0;
            return;
          }
        }
        obj->cmdValue=atof(obj->cmdPara);
        memset(obj->cmdPara,0,sizeof(obj->cmdPara));
    }
}
void comDispatch(cmdHandle handle)
{
    serialCMD *obj= (serialCMD *)handle;

        if(obj->flagNewcmd)
        {
            switch(obj->cmdName[0])
            {
                case 'F':
                    break;
                case 'B': //Backward
                    break;
                case 'L':// Left turn
                    break;
                case 'R': // Right turn
                    break;
                case 'C': //clear setting
                    break;
                default:
                    break;
            }
            obj->flagNewcmd=0;
        }
}





