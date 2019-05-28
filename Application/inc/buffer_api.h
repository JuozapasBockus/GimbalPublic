#ifndef _BUFFER_API_
#define _BUFFER_API_

#include <stdint.h>
#include <stdbool.h>


typedef enum {
    eBuffer_First,
    eBuffer_Uart1Rx = eBuffer_First,
    eBuffer_Uart1Tx,
    eBuffer_Last,
} eBuffer_t;

#ifdef WHATEVER
bool            BufferReadIndex_Increment       (eBuffer_t Buffer, unsigned int IncrementAmount);
unsigned int    BufferReadIndex_Get             (eBuffer_t Buffer);
bool            BufferReadIndex_Set             (eBuffer_t Buffer, unsigned int NewValue);
bool            BufferWriteIndex_Increment      (eBuffer_t Buffer, unsigned int IncrementAmount);
bool            BufferWriteIndex_Set            (eBuffer_t Buffer, unsigned int NewValue);
#endif
unsigned int    BufferWriteIndex_Get            (eBuffer_t Buffer);
bool            WriteByteToBuffer               (eBuffer_t Buffer, char NewByte);
char            ReadByteFromBuffer              (eBuffer_t Buffer);
unsigned int    GetMessageFromBuffer            (eBuffer_t Buffer, uint8_t ReadEnd, char *OutputBuffer, unsigned int MaxLength);
unsigned int    WriteMessageToBuffer            (eBuffer_t Buffer, char *InputBuffer, unsigned int Length);
unsigned int    UnreadMessagesInBuffer          (eBuffer_t Buffer);
bool            BufferIsEmpty                   (eBuffer_t Buffer);

#endif /* _BUFFER_API_ */
