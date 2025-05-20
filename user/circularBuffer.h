#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H
#include <stdint.h>

#define DATA_TYPE float
// Circular buffer structure
typedef struct circular_buffer {
    uint16_t read_index;
    uint16_t write_index;
    uint16_t length;
    DATA_TYPE *buffer;
} cirBuffer_t, *pCirBuffer;

// Function prototypes
// Initialize the circular buffer
void cirBufferInit(pCirBuffer pBuffer, uint16_t length, DATA_TYPE *buffer);
int cirBufferRead(pCirBuffer pBuffer, DATA_TYPE *value);   
int cirBufferWrite(pCirBuffer pBuffer, DATA_TYPE value);

#endif


