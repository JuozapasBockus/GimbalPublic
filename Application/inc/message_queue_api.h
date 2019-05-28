#ifndef _UART_TASK_
#define _UART_TASK_

#include <stdint.h>
#include <stdbool.h>
#include "buffer_api.h"

typedef enum {
    eQueue_First,
    /* Queues for UART */
    eQueue_Uart_First = eQueue_First,
        eQueue_Uart1 = eQueue_Uart_First,
        eQueue_Uart3,
    eQueue_Uart_Last = eQueue_Uart3,
    /* Queues for SPI */
    eQueue_Spi_First,
        eQueue_Spi1Rx = eQueue_Spi_First,
        eQueue_Spi1Tx,
    eQueue_Spi_Last = eQueue_Spi1Tx,
    /* This entry must be last */
    eQueue_Last,
} eQueue_t;


void InitializeMessageQueues (void);
bool SendMessageToQueueFromISR (eQueue_t Queue, eBuffer_t Buffer, uint8_t ReadEnd);
bool ReceiveMessageFromQueue (eQueue_t Queue, char *OutputBuffer, unsigned int MaxLength);
bool SendByteToQueue (eQueue_t Queue, uint8_t InputByte);
bool ReceiveByteFromQueue (eQueue_t Queue, uint8_t *OuptutBytePtr);
bool SendByteToQueueFromISR (eQueue_t Queue, uint8_t InputByte);
bool ReceiveByteFromQueueFromISR (eQueue_t Queue, uint8_t *OuptutBytePtr);
unsigned int MessagesPresentInQueueFromISR (eQueue_t Queue);

#endif /* _UART_TASK_ */
