#include "Spi_api.h"

#include <stdbool.h>
#include <stdint.h>
#include "gpio.h"
#include "Spi.h"
#include "FreeRTOS.h"
#include "Task.h"
#include "semphr.h"
#include "stm32f3xx_it.h"
#include "stm32f3xx_ll_Spi.h"
#include "message_queue_api.h"
#include "uart_api.h"
#include "error_handling_api.h"

#define SPI_READ_REQUEST_BIT 0x80
#define SPI_WRITE_REQUEST_BIT 0x7F

struct {
    eSpi_t          Spi;
    GPIO_TypeDef*   SlaveCsGpioPort;
    uint16_t        SlaveCsGpioPin;
} sSpiSlaveDescriptor[eSpiSlave_Last] = {
    [eSpiSlave_MPU]     = {eSpi_1, GPIOA, GPIO_PIN_4},
    [eSpiSlave_None]    = {eSpi_Last, NULL, 0}
};

typedef enum {
    eSpiDataSize_8Bit,
    eSpiDataSize_16Bit,
} eSpiDataSize_t;

struct {
    SPI_TypeDef *SpiPeriphPtr;
    eQueue_t RxQueue;
    eQueue_t TxQueue;
    eSpiSlave_t SelectedSlave;
    const bool Active;
    SemaphoreHandle_t Mutex;
} SpiDescriptor[eSpi_Last] = {
    [eSpi_1] = {SPI1, eQueue_Spi1Rx, eQueue_Spi1Tx, eSpiSlave_None, true, NULL},
    [eSpi_2] = {SPI2, eQueue_Last, eQueue_Last, eSpiSlave_Last, false, NULL},
};

void InitializeSpiMutexes (void) {
    for (eSpi_t i = eSpi_First; i < eSpi_Last; i++) {
        if (SpiDescriptor[i].Active) {
            if (SpiDescriptor[i].Mutex == NULL) {
                SpiDescriptor[i].Mutex = xSemaphoreCreateMutex();
            } else {
                /* TODO: handle double initialization */
            }
        }
    }
}

void HandleSpiRxIRQ (eSpi_t CurrentSpi) {
    /* Input check */
    if (CurrentSpi < eSpi_Last) {
        if (LL_SPI_IsActiveFlag_RXNE(SpiDescriptor[CurrentSpi].SpiPeriphPtr)) {
            uint8_t ReceivedByte = LL_SPI_ReceiveData8(SpiDescriptor[CurrentSpi].SpiPeriphPtr);
            SendByteToQueueFromISR(SpiDescriptor[CurrentSpi].RxQueue, ReceivedByte);
        } else {
            
        }
    }
}

void HandleSpiTxIRQ (eSpi_t CurrentSpi) {
    /* Input check */
    if (CurrentSpi < eSpi_Last) {
        if (LL_SPI_IsActiveFlag_TXE(SpiDescriptor[CurrentSpi].SpiPeriphPtr)) {
            if (MessagesPresentInQueueFromISR(SpiDescriptor[CurrentSpi].TxQueue)) {
                uint8_t ByteToSend = 0;
                if (ReceiveByteFromQueueFromISR(SpiDescriptor[CurrentSpi].TxQueue, &ByteToSend)) {
                    LL_SPI_TransmitData8(SpiDescriptor[CurrentSpi].SpiPeriphPtr, ByteToSend);
                }
            } else {
                LL_SPI_DisableIT_TXE(SpiDescriptor[CurrentSpi].SpiPeriphPtr);
            }
        }
    }
}

bool DeselectSpiSlave (eSpi_t TargetSpi) {
    bool RetVal = false;
    /* Input check */
    if ((TargetSpi < eSpi_Last)) {
        HAL_GPIO_WritePin(sSpiSlaveDescriptor[SpiDescriptor[TargetSpi].SelectedSlave].SlaveCsGpioPort,
                          sSpiSlaveDescriptor[SpiDescriptor[TargetSpi].SelectedSlave].SlaveCsGpioPin, GPIO_PIN_SET);
        SpiDescriptor[TargetSpi].SelectedSlave = eSpiSlave_None;
        RetVal = true;
    }
    return RetVal;
}

bool SelectSpiSlave (eSpi_t TargetSpi, eSpiSlave_t NewSlave) {
    bool RetVal = false;
    /* Input check */
    if ((NewSlave < eSpiSlave_Last) && (TargetSpi < eSpi_Last)) {
        if (NewSlave != eSpiSlave_None) {
            if (SpiDescriptor[TargetSpi].SelectedSlave == eSpiSlave_None) {
                HAL_GPIO_WritePin(sSpiSlaveDescriptor[NewSlave].SlaveCsGpioPort,
                                  sSpiSlaveDescriptor[NewSlave].SlaveCsGpioPin, GPIO_PIN_RESET);
                SpiDescriptor[TargetSpi].SelectedSlave = NewSlave;
                RetVal = true;
            }
        } else {
            RetVal = DeselectSpiSlave(TargetSpi);
        }
    }
    return RetVal;
}

bool ReselectSpiSlave (eSpi_t TargetSpi) {
    bool RetVal = false;
    /* Input check */
    if ((TargetSpi < eSpi_Last)) {
        HAL_GPIO_WritePin(sSpiSlaveDescriptor[SpiDescriptor[TargetSpi].SelectedSlave].SlaveCsGpioPort,
                          sSpiSlaveDescriptor[SpiDescriptor[TargetSpi].SelectedSlave].SlaveCsGpioPin, GPIO_PIN_SET);
        vTaskDelay(10);
        HAL_GPIO_WritePin(sSpiSlaveDescriptor[SpiDescriptor[TargetSpi].SelectedSlave].SlaveCsGpioPort,
                          sSpiSlaveDescriptor[SpiDescriptor[TargetSpi].SelectedSlave].SlaveCsGpioPin, GPIO_PIN_RESET);
        RetVal = true;
    }
    return RetVal;
}

bool SpiSendByte (eSpi_t CurrentSpi, uint8_t ByteToSend) {
    bool RetVal = false;
    /* Input check */
    if (CurrentSpi < eSpi_Last) {
        if(SendByteToQueue(SpiDescriptor[CurrentSpi].TxQueue, ByteToSend)) {
            if (!LL_SPI_IsEnabledIT_TXE(SpiDescriptor[CurrentSpi].SpiPeriphPtr)) {
                LL_SPI_EnableIT_TXE(SpiDescriptor[CurrentSpi].SpiPeriphPtr);
            }
            RetVal = true;
        }
    }
    return RetVal;
}

bool SpiReceiveByte (eSpi_t CurrentSpi, uint8_t *ByteToReceivePtr) {
    bool RetVal = false;
    /* Input check */
    if (CurrentSpi < eSpi_Last) {
        if (ReceiveByteFromQueue(SpiDescriptor[CurrentSpi].RxQueue, ByteToReceivePtr)) {
            RetVal = true;
        }
    }
    return RetVal;
}

bool SpiReadRequest (eSpi_t TargetSpi,  uint8_t *Response, uint8_t Request) {
    bool RetVal = false;
    uint8_t DataToBeDiscarded = 0;
    /* input check */
    if ((TargetSpi < eSpi_Last) && (Response != NULL)) {
        if (SpiSendByte(TargetSpi, Request)) {
            SpiReceiveByte(TargetSpi, &DataToBeDiscarded);//useless data
            SpiSendByte(TargetSpi, 0);//dummy data
            if (SpiReceiveByte(TargetSpi, Response)) {
                RetVal = true;
            }
        }
    }
    return RetVal;
}

bool SpiWriteRequest (eSpi_t TargetSpi, uint8_t Value, uint8_t Address) {
    bool RetVal = false;
    uint8_t DataToBeDiscarded = 0;
    uint8_t DataCheckBuffer = 0;
    /* input check */
    if (TargetSpi < eSpi_Last) {
        if (SpiSendByte(TargetSpi, Address)) {
            SpiReceiveByte(TargetSpi, &DataToBeDiscarded);//useless data
            SpiSendByte(TargetSpi, Value);
            SpiReceiveByte(TargetSpi, &DataToBeDiscarded);//useless data
            /* Read same register to make sure that data has been written correctly */
            ReselectSpiSlave(TargetSpi);
            if (SpiReadRequest(TargetSpi, &DataCheckBuffer, (Address | SPI_READ_REQUEST_BIT))) {
                if (DataCheckBuffer == Value) {
                    RetVal = true;
                } else {
                    PrintToUart(eUart_1, "ERROR[SPI]: Written value mismatch detected\r");
                }
            }
        }
    }
    return RetVal;
}

void SpiEnable (eSpi_t Spi) {
    LL_SPI_Enable(SpiDescriptor[Spi].SpiPeriphPtr);
    LL_SPI_EnableIT_RXNE(SpiDescriptor[Spi].SpiPeriphPtr);
}

void SpiDisable (eSpi_t Spi) {
    LL_SPI_DisableIT_RXNE(SpiDescriptor[Spi].SpiPeriphPtr);
    LL_SPI_Disable(SpiDescriptor[Spi].SpiPeriphPtr);
}

//#define BYPASS_MUTEX

bool SpiReadSlaveRegister (uint8_t *DataResponse, uint8_t RegisterAddress, eSpiSlave_t Slave) {
    bool RetVal = false;
    RegisterAddress |= SPI_READ_REQUEST_BIT;
    /* Check input */
    if ((Slave < eSpiSlave_Last) && (SpiDescriptor[sSpiSlaveDescriptor[Slave].Spi].Mutex != NULL)) {
#ifndef BYPASS_MUTEX
        if (xSemaphoreTake(SpiDescriptor[sSpiSlaveDescriptor[Slave].Spi].Mutex, portMAX_DELAY) == pdTRUE) {
#endif
            SpiEnable(sSpiSlaveDescriptor[Slave].Spi);
            if (SelectSpiSlave (sSpiSlaveDescriptor[Slave].Spi, Slave)) {
                if (SpiReadRequest(sSpiSlaveDescriptor[Slave].Spi, DataResponse, RegisterAddress)) {
                    RetVal = true;
                } else {
                    //PrintToUart(eUart_1, "ERROR[SPI%d]: data request failed\r", sSpiSlaveDescriptor[Slave].Spi+1);
                }
            } else {
                //PrintToUart(eUart_1, "ERROR[SPI%d]: Slave(%d) select failed\r", sSpiSlaveDescriptor[Slave].Spi+1, Slave);
            }
            if (!DeselectSpiSlave (eSpi_1)) {
                //PrintToUart(eUart_1, "ERROR[SPI%d]: Slave(%d) deselect failed\r", sSpiSlaveDescriptor[Slave].Spi+1, Slave);
            }
            SpiDisable(sSpiSlaveDescriptor[Slave].Spi);
#ifndef BYPASS_MUTEX
            xSemaphoreGive(SpiDescriptor[sSpiSlaveDescriptor[Slave].Spi].Mutex);
        } else {
            /* TODO: handle timeout */
        }
#endif
    }
    return RetVal;
}

bool SpiWriteSlaveRegister (uint8_t WriteValue, uint8_t RegisterAddress, eSpiSlave_t Slave) {
    bool RetVal = false;
    RegisterAddress &= SPI_WRITE_REQUEST_BIT;
    /* Check input */
    if ((Slave < eSpiSlave_Last) && (SpiDescriptor[sSpiSlaveDescriptor[Slave].Spi].Mutex != NULL)){
#ifndef BYPASS_MUTEX
        if (xSemaphoreTake(SpiDescriptor[sSpiSlaveDescriptor[Slave].Spi].Mutex, portMAX_DELAY) == pdTRUE) {
#endif
            SpiEnable(sSpiSlaveDescriptor[Slave].Spi);
            if (SelectSpiSlave (sSpiSlaveDescriptor[Slave].Spi, Slave)) {
                if (SpiWriteRequest(sSpiSlaveDescriptor[Slave].Spi, WriteValue, RegisterAddress)) {
                    RetVal = true;
                } else {
                    //PrintToUart(eUart_1, "ERROR[SPI%d]: data request failed\r", sSpiSlaveDescriptor[Slave].Spi);
                }
            } else {
                //PrintToUart(eUart_1, "ERROR[SPI%d]: Slave(%d) select failed\r", sSpiSlaveDescriptor[Slave].Spi, Slave);
            }
            if (!DeselectSpiSlave (eSpi_1)) {
                //PrintToUart(eUart_1, "ERROR[SPI%d]: Slave(%d) deselect failed\r", sSpiSlaveDescriptor[Slave].Spi, Slave);
            }
            SpiDisable(sSpiSlaveDescriptor[Slave].Spi);
#ifndef BYPASS_MUTEX
            xSemaphoreGive(SpiDescriptor[sSpiSlaveDescriptor[Slave].Spi].Mutex);
        } else {
            /* Handle Timeout */
        }
#endif
    }
    return RetVal;
}
