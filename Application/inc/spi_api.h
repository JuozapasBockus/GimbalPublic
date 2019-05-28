#ifndef _SPI_API_
#define _SPI_API_

#include <stdbool.h>
#include <stdint.h>


typedef enum {
    eSpi_First,
    eSpi_1 = eSpi_First,
    eSpi_2,
    eSpi_3,
    eSpi_Last,
} eSpi_t;

typedef enum {
    eSpiSlave_First,
    eSpiSlave_MPU = eSpiSlave_First,
    /* These two must be last */
    eSpiSlave_None,
    eSpiSlave_Last,
} eSpiSlave_t;

void InitializeSpiMutexes (void);
void HandleSpiRxIRQ (eSpi_t CurrentSpi);
void HandleSpiTxIRQ (eSpi_t CurrentSpi);
bool SpiReadSlaveRegister (uint8_t *DataResponse, uint8_t RegisterAddress, eSpiSlave_t Slave);
bool SpiWriteSlaveRegister (uint8_t WriteValue, uint8_t RegisterAddress, eSpiSlave_t Slave);

#endif /* _SPI_API_ */

