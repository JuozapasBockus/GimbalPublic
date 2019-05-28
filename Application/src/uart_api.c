#include "uart_api.h"

#include <stdarg.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32f3xx_it.h"
#include "stm32f3xx_ll_usart.h"
#include "buffer_api.h"
#include "message_queue_api.h"


struct {
    USART_TypeDef *UartPeriphPtr;
    eQueue_t Queue;
    eBuffer_t RxBuffer;
    eBuffer_t TxBuffer;
    const bool Active;
    SemaphoreHandle_t Mutex;
} UartDescriptor[eUart_Last] = {
    [eUart_1] = {USART1, eQueue_Uart1, eBuffer_Uart1Rx, eBuffer_Uart1Tx, true, NULL},
    [eUart_2] = {USART2, eQueue_Last, eBuffer_Last, eBuffer_Last, false, NULL},
    [eUart_3] = {USART3, eQueue_Last, eBuffer_Last, eBuffer_Last, false, NULL},
};


void InitializeUartMutexes (void) {
    for (eUart_t i = eUart_First; i < eUart_Last; i++) {
        if (UartDescriptor[i].Active) {
            if (UartDescriptor[i].Mutex == NULL) {
                UartDescriptor[i].Mutex = xSemaphoreCreateMutex();
            } else {
                /* TODO: handle double initialization */
            }
        }
    }
}

void InitializeUartInterrupts (void) {
    for (eUart_t i = eUart_First; i < eUart_Last; i++) {
        if (UartDescriptor[i].Active) {
            LL_USART_EnableIT_TXE(UartDescriptor[i].UartPeriphPtr);
            LL_USART_EnableIT_RXNE(UartDescriptor[i].UartPeriphPtr);
            LL_USART_Enable(UartDescriptor[i].UartPeriphPtr);
        }
    }
}



void HandleUartRxIRQ (eUart_t CurrentUart) {
    if (CurrentUart < eUart_Last) {
        if (LL_USART_IsActiveFlag_RXNE(UartDescriptor[CurrentUart].UartPeriphPtr)) {
            char NewByte = (char)LL_USART_ReceiveData8(UartDescriptor[CurrentUart].UartPeriphPtr);
            uint8_t CurrentWriteIndex = BufferWriteIndex_Get(UartDescriptor[CurrentUart].RxBuffer);
            if (WriteByteToBuffer(UartDescriptor[CurrentUart].RxBuffer, NewByte)) {
                if (NewByte == '\r') {
                    SendMessageToQueueFromISR(UartDescriptor[CurrentUart].Queue, UartDescriptor[CurrentUart].RxBuffer, CurrentWriteIndex);
                }
            }
        }
    }
}

void HandleUartTxIRQ (eUart_t CurrentUart) {
    /* Input check */
    if (CurrentUart < eUart_Last) {
        if (LL_USART_IsActiveFlag_TXE(UartDescriptor[CurrentUart].UartPeriphPtr)) {
            if (!BufferIsEmpty(UartDescriptor[CurrentUart].TxBuffer)) {
                char OutputByte = ReadByteFromBuffer(UartDescriptor[CurrentUart].TxBuffer);
                if (OutputByte != '\0') {
                    LL_USART_TransmitData8(UartDescriptor[CurrentUart].UartPeriphPtr, (uint8_t)OutputByte);
                }
            } else {
                LL_USART_DisableIT_TXE(UartDescriptor[CurrentUart].UartPeriphPtr);
            }
        }
    }
}

//#define BYPASS_MUTEX

bool PrintToUart(eUart_t OutputUart, char *Format, ...) {
    #define MAX_MESSAGE_LENGTH 255
    bool RetVal = false;
    char LocalBuffer[MAX_MESSAGE_LENGTH];
    int WrittenLength = 0;
    va_list args;
    va_start (args, Format);
    WrittenLength = vsnprintf (LocalBuffer, MAX_MESSAGE_LENGTH, Format, args);
    va_end (args);
    if ((WrittenLength > 0) && (WrittenLength < MAX_MESSAGE_LENGTH && UartDescriptor[OutputUart].Mutex)) {
#ifndef BYPASS_MUTEX
        if (xSemaphoreTake(UartDescriptor[OutputUart].Mutex, portMAX_DELAY) == pdTRUE) {
#endif
            if (WriteMessageToBuffer(UartDescriptor[OutputUart].TxBuffer, LocalBuffer, WrittenLength)) {
                if (!LL_USART_IsEnabledIT_TXE(UartDescriptor[OutputUart].UartPeriphPtr)) {
                    LL_USART_EnableIT_TXE(UartDescriptor[OutputUart].UartPeriphPtr);
                }
                RetVal = true;
            }
#ifndef BYPASS_MUTEX
            xSemaphoreGive(UartDescriptor[OutputUart].Mutex);
        } else {
            /* TODO: handle timeout */
        }
#endif
    }
    return RetVal;
    #undef MAX_MESSAGE_LENGTH
}

bool ReceiveMessageFromUart(eUart_t InputUart, char *OutputBuffer, unsigned int MaxLength) {
    bool RetVal = false;
    if (ReceiveMessageFromQueue(UartDescriptor[InputUart].Queue, OutputBuffer, MaxLength)) {
        RetVal = true;
    }
    return RetVal;
}
