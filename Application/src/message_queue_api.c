#include "message_queue_api.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "buffer_api.h"
#include "error_handling_api.h"


/* TODO: figure this out */
#define HARDCODED_MESSAGE_QUEUE_SIZE 16
#define HARDCODED_QUEUE_TIMEOUT 50

typedef struct {
    eBuffer_t Buffer;
    uint8_t ReadEnd;
} sMessageQueueItem_t;

struct {
    QueueHandle_t QueueHandle;
    unsigned int DataSize;
} QueueDescriptor[eQueue_Last] = {
    [eQueue_Uart1]      = { NULL,   sizeof(sMessageQueueItem_t) },
    [eQueue_Spi1Rx]     = { NULL,   sizeof(uint8_t)             },
    [eQueue_Spi1Tx]     = { NULL,   sizeof(uint8_t)             }
};


void InitializeMessageQueues (void) {
    for (eQueue_t i = eQueue_First; i < eQueue_Last; i++) {
        if (QueueDescriptor[i].QueueHandle == NULL) {
            QueueDescriptor[i].QueueHandle = xQueueCreate(HARDCODED_MESSAGE_QUEUE_SIZE, QueueDescriptor[i].DataSize);
        } else {
            /* TODO: handle double initialization */
            ReportError();
        }
    }
}

bool SendMessageToQueueFromISR(eQueue_t Queue, eBuffer_t Buffer, uint8_t ReadEnd) {
    bool RetVal = false;
    /* Input check */
    if ((Queue < eQueue_Last) && (Buffer < eBuffer_Last) && (QueueDescriptor[Queue].QueueHandle != NULL) &&
        (QueueDescriptor[Queue].DataSize == sizeof(sMessageQueueItem_t))) {
        sMessageQueueItem_t MessageItem = {Buffer, ReadEnd};
        const void *InputPtr = &MessageItem;
        /* TODO: handle timeout */
        /* TODO: check queue for space */
        if (xQueueSendFromISR(QueueDescriptor[Queue].QueueHandle, InputPtr, NULL) == pdTRUE) {
            RetVal = true;
        }
    }
    return RetVal;
}

bool ReceiveMessageFromQueue(eQueue_t Queue, char *OutputBuffer, unsigned int MaxLength) {
    bool RetVal = false;
    /* Input check */
    if ((Queue < eQueue_Last) && (OutputBuffer != NULL) && MaxLength && (QueueDescriptor[Queue].QueueHandle != NULL) &&
        (QueueDescriptor[Queue].DataSize == sizeof(sMessageQueueItem_t))) {
        sMessageQueueItem_t MessageItem = {eBuffer_Last, 0};
        void *OutputPtr = &MessageItem;
        /* TODO: handle timeout */
        if (xQueueReceive(QueueDescriptor[Queue].QueueHandle, OutputPtr, HARDCODED_QUEUE_TIMEOUT) == pdTRUE) {
            if (GetMessageFromBuffer (MessageItem.Buffer, MessageItem.ReadEnd, OutputBuffer, MaxLength)) {
                RetVal = true;
            }
        }
    }
    return RetVal;
}

bool SendByteToQueue(eQueue_t Queue, uint8_t InputByte) {
    bool RetVal = false;
    /* Input check */
    if ((Queue < eQueue_Last) && (QueueDescriptor[Queue].QueueHandle != NULL) &&
        (QueueDescriptor[Queue].DataSize == sizeof(uint8_t))) {
        /* TODO: handle timeout */
        /* TODO: check queue for space */
        if (xQueueSend(QueueDescriptor[Queue].QueueHandle, &InputByte, HARDCODED_QUEUE_TIMEOUT) == pdTRUE) {
            RetVal = true;
        }
    }
    return RetVal;
}

bool ReceiveByteFromQueue(eQueue_t Queue, uint8_t *OuptutBytePtr) {
    bool RetVal = false;
    /* Input check */
    if ((Queue < eQueue_Last) && (OuptutBytePtr != NULL) && (QueueDescriptor[Queue].QueueHandle != NULL) &&
        (QueueDescriptor[Queue].DataSize == sizeof(uint8_t))) {
        /* TODO: handle timeout */
        if (xQueueReceive(QueueDescriptor[Queue].QueueHandle, OuptutBytePtr, HARDCODED_QUEUE_TIMEOUT) == pdTRUE) {
            RetVal = true;
        }
    }
    return RetVal;
}

bool SendByteToQueueFromISR(eQueue_t Queue, uint8_t InputByte) {
    bool RetVal = false;
    /* Input check */
    if ((Queue < eQueue_Last) && (QueueDescriptor[Queue].QueueHandle != NULL) &&
        (QueueDescriptor[Queue].DataSize == sizeof(uint8_t))) {
        uint8_t *InputPtr = &InputByte;
        /* TODO: handle timeout */
        /* TODO: check queue for space */
        if (xQueueSendFromISR(QueueDescriptor[Queue].QueueHandle, InputPtr, NULL) == pdTRUE) {
            RetVal = true;
        }
    }
    return RetVal;
}

bool ReceiveByteFromQueueFromISR(eQueue_t Queue, uint8_t *OuptutBytePtr) {
    bool RetVal = false;
    /* Input check */
    if ((Queue < eQueue_Last) && (OuptutBytePtr != NULL) && (QueueDescriptor[Queue].QueueHandle != NULL) &&
        (QueueDescriptor[Queue].DataSize == sizeof(uint8_t))) {
        /* TODO: handle timeout */
        if (xQueueReceiveFromISR(QueueDescriptor[Queue].QueueHandle, OuptutBytePtr, NULL) == pdTRUE) {
            RetVal = true;
        }
    }
    return RetVal;
}

unsigned int MessagesPresentInQueueFromISR (eQueue_t Queue) {
    unsigned int RetVal = 0;
    if ((Queue < eQueue_Last) && (QueueDescriptor[Queue].QueueHandle != NULL)) {
        RetVal = uxQueueMessagesWaitingFromISR(QueueDescriptor[Queue].QueueHandle);
    }
    return RetVal;
}
