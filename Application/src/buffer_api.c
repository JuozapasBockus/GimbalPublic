#include "buffer_api.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#define UART_BUFFER_SIZE 256


static char g_Uart1RxBuffer[UART_BUFFER_SIZE];
static char g_Uart1TxBuffer[UART_BUFFER_SIZE];

struct {
    char *BufferPointer;
    const unsigned int BufferSize;
    uint8_t WriteIndex;
    uint8_t ReadIndex;
} sBufferController[eBuffer_Last] = {
    [eBuffer_Uart1Rx] = {   g_Uart1RxBuffer,  UART_BUFFER_SIZE,   0,  0 },
    [eBuffer_Uart1Tx] = {   g_Uart1TxBuffer,  UART_BUFFER_SIZE,   0,  0 }
};
/* This code is PROBABLY not useful anymore */
#ifdef WHATEVER
bool BufferReadIndex_Increment (eBuffer_t Buffer, unsigned int IncrementAmount) {
    bool RetVal = false;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        sBufferController[Buffer].ReadIndex += IncrementAmount;
        RetVal = true;
    }
    return RetVal;
}

unsigned int BufferReadIndex_Get (eBuffer_t Buffer) {
    unsigned int RetVal = 0;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        RetVal = sBufferController[Buffer].ReadIndex;
    }
    return RetVal;
}

bool BufferReadIndex_Set (eBuffer_t Buffer, unsigned int NewValue) {
    bool RetVal = false;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        sBufferController[Buffer].ReadIndex = NewValue;
        RetVal = true;
    }
    return RetVal;
}

bool BufferWriteIndex_Increment (eBuffer_t Buffer, unsigned int IncrementAmount) {
    bool RetVal = false;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        sBufferController[Buffer].WriteIndex += IncrementAmount;
        RetVal = true;
    }
    return RetVal;
}

bool BufferWriteIndex_Set (eBuffer_t Buffer, unsigned int NewValue) {
    bool RetVal = false;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        sBufferController[Buffer].WriteIndex = NewValue;
        RetVal = true;
    }
    return RetVal;
}
#endif

unsigned int BufferWriteIndex_Get (eBuffer_t Buffer) {
    unsigned int RetVal = 0;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        RetVal = sBufferController[Buffer].WriteIndex;
    }
    return RetVal;
}

bool WriteByteToBuffer (eBuffer_t Buffer, char NewByte) {
    bool RetVal = false;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        /* Ignore these hardcoded chars */
        if (!strchr("\n \t\0", NewByte)) {
            sBufferController[Buffer].BufferPointer[sBufferController[Buffer].WriteIndex] = NewByte;
            sBufferController[Buffer].WriteIndex++;
        }
        RetVal = true;
    }
    return RetVal;
}

char ReadByteFromBuffer (eBuffer_t Buffer) {
    char RetVal = '\0';
    /* Input check */
    if (Buffer < eBuffer_Last) {
        RetVal = sBufferController[Buffer].BufferPointer[sBufferController[Buffer].ReadIndex];
        sBufferController[Buffer].ReadIndex++;
    }
    return RetVal;
}

unsigned int GetMessageFromBuffer (eBuffer_t Buffer, uint8_t ReadEnd, char *OutputBuffer, unsigned int MaxLength) {
    unsigned int i = 0;
    /* Input check */
    if (Buffer < eBuffer_Last) {
        while ((i != ReadEnd) && (i < MaxLength) && (i < sBufferController[Buffer].BufferSize)) {
            OutputBuffer[i] = sBufferController[Buffer].BufferPointer[sBufferController[Buffer].ReadIndex];
            i++;
            sBufferController[Buffer].ReadIndex++;
        }
    }
    /* Return number of bytes read */
    return i;
}

unsigned int WriteMessageToBuffer (eBuffer_t Buffer, char *InputBuffer, unsigned int Length) {
    unsigned int i = 0;
    /* Input check */
    if ((Buffer < eBuffer_Last) && (Length < (sBufferController[Buffer].BufferSize - 1))) {
        while ((i < Length)) {
            sBufferController[Buffer].BufferPointer[sBufferController[Buffer].WriteIndex] = InputBuffer[i];
            i++;
            sBufferController[Buffer].WriteIndex++;
        }
    }
    /* Return number of bytes written */
    return i;
}

bool BufferIsEmpty (eBuffer_t Buffer) {
    bool RetVal = false;
    if (sBufferController[Buffer].WriteIndex == sBufferController[Buffer].ReadIndex) {
        RetVal = true;
    } else {
        RetVal = false;
    }
    return RetVal;
}
