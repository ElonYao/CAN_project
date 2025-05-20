#include "circularBuffer.h"


void cirBufferInit(pCirBuffer pBuffer, uint16_t length, DATA_TYPE *buffer)
{
    pBuffer->read_index = 0;
    pBuffer->write_index = 0;
    pBuffer->length = length;
    pBuffer->buffer = buffer;
}
int cirBufferRead(pCirBuffer pBuffer, DATA_TYPE *value)
{
    if (pBuffer->read_index != pBuffer->write_index)
    {
        *value = pBuffer->buffer[pBuffer->read_index];
        pBuffer->read_index++;
        if (pBuffer->read_index == pBuffer->length)
            pBuffer->read_index = 0;
        return 0; // Success
    }
    else
    {
        return -1; // Buffer empty
    }
}
int cirBufferWrite(pCirBuffer pBuffer, DATA_TYPE value)
{
    uint16_t next_write_index;
    
    next_write_index = pBuffer->write_index + 1;
    if (next_write_index == pBuffer->length)
        next_write_index = 0;

    if (next_write_index != pBuffer->read_index)
    {
        pBuffer->buffer[pBuffer->write_index] = value;
        pBuffer->write_index = next_write_index;
        return 0; // Success
    }
    else
    {
        return -1; // Buffer full
    }
}
// End of circularBuffer.c
