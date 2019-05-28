#include "mpu9250_api.h"

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "Spi_api.h"
#include "uart_api.h"
#include "error_handling_api.h"

extern osThreadId defaultTaskHandle;

#define SHIFT_TO_H(x) (x << 8)

/* Hardcoded responses */
#define WHO_AM_I_RESPONSE 0x71

/* Hardcoded config values */
#define HARDCODED_GYR_CONFIG        0x10 //GYRO_FS_SEL = +500 dps, rest is default
#define HARDCODED_ACC_CONFIG        0x10 //ACCEL_FS_SEL = +-8 g, rest is default

#define HARDCODED_INT_PIN           0x10 //INT cleared by reading any data
#define HARDCODED_INT_EN            0x01 //INT only on raw data ready
#ifdef USE_FIFO
#define HARDCODED_FIFO_ENABLE       0x78 //all ACC and GYRO data included in FIFO
#define HARDCODED_USER_CTRL         0x40 //FIFO enalbed
#else
#define HARDCODED_FIFO_ENABLE       0x00
#define HARDCODED_USER_CTRL         0x00
#endif

#define HARDCODED_I2C_CTRL          0x40 //INT delayed by external sensor
#define AKM_DEVICE_ID               0x48
#define AKM_ADDRESS_TO_READ         0x03
#define AKM_BYTES_TO_READ           6


/* Hardcoded conversion coeficients */
#define HARDCODED_ACC_CONV_COEF    (1/4.096)
#define HARDCODED_GYR_CONV_COEF    (1/32.8)
#define HARDCODED_MAG_CONV_COEF    (1/0.6)


typedef enum {
    eMpuRegisters_SmplRt_div    =   0x19,
    eMpuRegisters_Config        =   0x1A,
    eMpuRegisters_GConfig       =   0x1B,
    eMpuRegisters_AConfig1      =   0x1C,
    eMpuRegisters_AConfig2      =   0x1D,
    eMpuRegisters_AOdr          =   0x1E,
    eMpuRegisters_Wom_Thr       =   0x1F,

    eMpuRegisters_FifoEn        =   0x23,
    eMpuRegisters_I2CCtrl       =   0x24,
    eMpuRegisters_slave0Addr    =   0x25,
    eMpuRegisters_slave0Rer     =   0x26,
    eMpuRegisters_slave0Ctrl    =   0x27,

    eMpuRegisters_IntPinCfg     =   0x37,
    eMpuRegisters_IntEn         =   0x38,

    eMpuRegisters_IntStatus     =   0x3A,
    eMpuRegisters_AXH           =   0x3B,
    eMpuRegisters_AXL           =   0x3C,
    eMpuRegisters_AYH           =   0x3D,
    eMpuRegisters_AYL           =   0x3E,
    eMpuRegisters_AZH           =   0x3F,
    eMpuRegisters_AZL           =   0x40,

    eMpuRegisters_GXH           =   0x43,
    eMpuRegisters_GXL           =   0x44,
    eMpuRegisters_GYH           =   0x45,
    eMpuRegisters_GYL           =   0x46,
    eMpuRegisters_GZH           =   0x47,
    eMpuRegisters_GZL           =   0x48,
    /* Taken from external sensor data */
    eMpuRegisters_MXH           =   0x49,
    eMpuRegisters_MXL           =   0x4A,
    eMpuRegisters_MYH           =   0x4B,
    eMpuRegisters_MYL           =   0x4C,
    eMpuRegisters_MZH           =   0x4D,
    eMpuRegisters_MZL           =   0x4E,

    eMpuRegisters_UserCtrl      =   0x6A,

    eMpuRegisters_FifoCountH    =   0x72,
    eMpuRegisters_FifoCountL    =   0x73,
    eMpuRegisters_FifoRW        =   0x74,
    eMpuRegisters_WhoAmI        =   0x75,
    /* This must be last */
    eMpuRegisters_Last,
} eMpuRegisters_t;


typedef struct {
    int16_t X;
    int16_t Y;
    int16_t Z;
} sRawData3D_t;

typedef struct {
    sRawData3D_t A;
    sRawData3D_t G;
    sRawData3D_t M;
} sImuRawData_t;


bool Mpu_Detect (void) {
    bool RetVal = false;
    uint8_t ResponseBuffer = 0;
    if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_WhoAmI, eSpiSlave_MPU)) {
        if (ResponseBuffer == WHO_AM_I_RESPONSE) {
            RetVal = true;
        } else {
            PrintToUart(eUart_1, "ERROR: MPU9250 not detected\r");
        }
    }
    return RetVal;
}

/* TODO (long term): improve structure of functions in this file */
bool Mpu_ARead (sRawData3D_t *AccData) {
    bool ErrorHasHappened = false;
    uint8_t ResponseBuffer = 0;
    /* X data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AXH, eSpiSlave_MPU)) {
            AccData->X = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AXL, eSpiSlave_MPU)) {
            AccData->X += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    /* Y data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AYH, eSpiSlave_MPU)) {
            AccData->Y = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AYL, eSpiSlave_MPU)) {
            AccData->Y += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    /* Z data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AZH, eSpiSlave_MPU)) {
            AccData->Z = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AZL, eSpiSlave_MPU)) {
            AccData->Z += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    if (ErrorHasHappened) {
        PrintToUart(eUart_1, "ERROR: Acc data reading failed\r");
    }
    return !ErrorHasHappened;
}

bool Mpu_GRead (sRawData3D_t *GyrData) {
    bool ErrorHasHappened = false;
    uint8_t ResponseBuffer = 0;
    /* X data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GXH, eSpiSlave_MPU)) {
            GyrData->X = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GXL, eSpiSlave_MPU)) {
            GyrData->X += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    /* Y data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GYH, eSpiSlave_MPU)) {
            GyrData->Y = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GYL, eSpiSlave_MPU)) {
            GyrData->Y += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    /* Z data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GZH, eSpiSlave_MPU)) {
            GyrData->Z = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GZL, eSpiSlave_MPU)) {
            GyrData->Z += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    if (ErrorHasHappened) {
        PrintToUart(eUart_1, "ERROR: Gyr data reading failed\r");
    }
    return !ErrorHasHappened;
}

bool Mpu_MRead (sRawData3D_t *GyrData) {
    bool ErrorHasHappened = false;
    uint8_t ResponseBuffer = 0;
    /* X data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_MXH, eSpiSlave_MPU)) {
            GyrData->X = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_MXL, eSpiSlave_MPU)) {
            GyrData->X += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    /* Y data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_MYH, eSpiSlave_MPU)) {
            GyrData->Y = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_MYL, eSpiSlave_MPU)) {
            GyrData->Y += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    /* Z data reading */
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_MZH, eSpiSlave_MPU)) {
            GyrData->Z = SHIFT_TO_H(ResponseBuffer);
        } else {
            ErrorHasHappened = true;
        }
    }
    if (!ErrorHasHappened) {
        if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_MZL, eSpiSlave_MPU)) {
            GyrData->Z += ResponseBuffer;
        } else {
            ErrorHasHappened = true;
        }
    }
    if (ErrorHasHappened) {
        PrintToUart(eUart_1, "ERROR: Gyr data reading failed\r");
    }
    return !ErrorHasHappened;
}


/* TODO: add mag data */
bool Mpu_ImuRead (sImuRawData_t *ImuRawData) {
    bool RetVal = false;
    if (Mpu_ARead(&(ImuRawData->A))) {
        if (Mpu_GRead(&(ImuRawData->G))) {
            if (Mpu_GRead(&(ImuRawData->M))) {
                RetVal = true;
            }
        }
    }
    return RetVal;
}

void Mpu_PrintRawData (sImuRawData_t *ImuRawData) {
    PrintToUart(eUart_1, "IMU data:\r\tAX\t%d\r\tAY\t%d\r\tAZ\t%d\r\tGX\t%d\r\tGY\t%d\r\tGZ\t%d\r\tMX\t%d\r\tMY\t%d\r\tMZ\t%d\r\r", 
                            ImuRawData->A.X, ImuRawData->A.Y, ImuRawData->A.Z,
                            ImuRawData->G.X, ImuRawData->G.Y, ImuRawData->G.Z,
                            ImuRawData->M.X, ImuRawData->M.Y, ImuRawData->M.Z);
}

void Mpu_PrintData (sImuData_t *ImuData) {
    PrintToUart(eUart_1, "IMU data:\r\tAX\t%f\r\tAY\t%f\r\tAZ\t%f\r\tGX\t%f\r\tGY\t%f\r\tGZ\t%f\r\tMX\t%f\r\tMY\t%f\r\tMZ\t%f\r\r", 
                            ImuData->A.X, ImuData->A.Y, ImuData->A.Z,
                            ImuData->G.X, ImuData->G.Y, ImuData->G.Z,
                            ImuData->M.X, ImuData->M.Y, ImuData->M.Z);
}

/* TODO: update or delete */
void Mpu_ConfigPrint (void) {
    uint8_t ResponseBuffer = 0;
    /* Read ODR */
    if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_SmplRt_div, eSpiSlave_MPU)) {
        PrintToUart(eUart_1, "MPU ODR:\t%x\r", ResponseBuffer);
    } else {
        PrintToUart(eUart_1, "MPU ODR \tERROR: read failed\r");
    }
    /* Read config */
    if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_Config, eSpiSlave_MPU)) {
        PrintToUart(eUart_1, "MPU CFG:\t%x\r", ResponseBuffer);
    } else {
        PrintToUart(eUart_1, "MPU CFG \tERROR: read failed\r");
    }
    /* Read gyro config */
    if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_GConfig, eSpiSlave_MPU)) {
        PrintToUart(eUart_1, "MPU GCFG:\t%x\r", ResponseBuffer);
    } else {
        PrintToUart(eUart_1, "MPU GCFG \tERROR: read failed\r");
    }
    /* Read acc1 config */
    if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AConfig1, eSpiSlave_MPU)) {
        PrintToUart(eUart_1, "MPU ACFG1:\t%x\r", ResponseBuffer);
    } else {
        PrintToUart(eUart_1, "MPU ACFG1 \tERROR: read failed\r");
    }
    /* Read acc2 config */
    if (SpiReadSlaveRegister(&ResponseBuffer, eMpuRegisters_AConfig2, eSpiSlave_MPU)) {
        PrintToUart(eUart_1, "MPU ACFG2:\t%x\r", ResponseBuffer);
    } else {
        PrintToUart(eUart_1, "MPU ACFG2 \tERROR: read failed\r");
    }
}

bool Mpu_Config (void) {
    bool ErrorHasHappened = false;
    /* set Gyro config*/
    if (!SpiWriteSlaveRegister(HARDCODED_GYR_CONFIG, eMpuRegisters_GConfig, eSpiSlave_MPU)) {
        //PrintToUart(eUart_1, "MPU Gyro config failed\r");
        ErrorHasHappened = true;
    }
    /* set Accel config*/
    if (!SpiWriteSlaveRegister(HARDCODED_ACC_CONFIG, eMpuRegisters_AConfig1, eSpiSlave_MPU)) {
        //PrintToUart(eUart_1, "MPU Accel config failed\r");
        ErrorHasHappened = true;
    }
    /* set Int pin config*/
    if (!SpiWriteSlaveRegister(HARDCODED_INT_PIN, eMpuRegisters_IntPinCfg, eSpiSlave_MPU)) {
        //PrintToUart(eUart_1, "MPU INT PIN config failed\r");
        ErrorHasHappened = true;
    }
    /* set Int en config*/
    if (!SpiWriteSlaveRegister(HARDCODED_INT_EN, eMpuRegisters_IntEn, eSpiSlave_MPU)) {
        //PrintToUart(eUart_1, "MPU INT EN config failed\r");
        ErrorHasHappened = true;
    }
    /* set FIFO en config*/
    if (!SpiWriteSlaveRegister(HARDCODED_FIFO_ENABLE, eMpuRegisters_FifoEn, eSpiSlave_MPU)) {
        //PrintToUart(eUart_1, "MPU FIFO en config failed\r");
        ErrorHasHappened = true;
    }
    /* set user ctrl*/
    if (!SpiWriteSlaveRegister(HARDCODED_USER_CTRL, eMpuRegisters_UserCtrl, eSpiSlave_MPU)) {
        //PrintToUart(eUart_1, "MPU user ctrl config failed\r");
        ErrorHasHappened = true;
    }
    /* set I2C master ctrl*/
    if (!SpiWriteSlaveRegister(HARDCODED_I2C_CTRL, eMpuRegisters_I2CCtrl, eSpiSlave_MPU)) {
        ErrorHasHappened = true;
    }
    /* set slave0 addr */
    if (!SpiWriteSlaveRegister(AKM_DEVICE_ID, eMpuRegisters_slave0Addr, eSpiSlave_MPU)) {
        ErrorHasHappened = true;
    }
    /* set slave0 reg */
    if (!SpiWriteSlaveRegister(AKM_ADDRESS_TO_READ, eMpuRegisters_slave0Rer, eSpiSlave_MPU)) {
        ErrorHasHappened = true;
    }
    /* set slave0 data length*/
    if (!SpiWriteSlaveRegister(AKM_BYTES_TO_READ, eMpuRegisters_slave0Ctrl, eSpiSlave_MPU)) {
        ErrorHasHappened = true;
    }
    return !ErrorHasHappened;
}

void Mpu_ConvertData (sImuData_t *ImuData, sImuRawData_t *ImuRawData) {
    ImuData->A.X = ImuRawData->A.X * HARDCODED_ACC_CONV_COEF;
    ImuData->A.Y = ImuRawData->A.Y * HARDCODED_ACC_CONV_COEF;
    ImuData->A.Z = ImuRawData->A.Z * HARDCODED_ACC_CONV_COEF;
    ImuData->G.X = ImuRawData->G.X * HARDCODED_GYR_CONV_COEF;
    ImuData->G.Y = ImuRawData->G.Y * HARDCODED_GYR_CONV_COEF;
    ImuData->G.Z = ImuRawData->G.Z * HARDCODED_GYR_CONV_COEF;
    ImuData->M.X = ImuRawData->M.X * HARDCODED_MAG_CONV_COEF;
    ImuData->M.Y = ImuRawData->M.Y * HARDCODED_MAG_CONV_COEF;
    ImuData->M.Z = ImuRawData->M.Z * HARDCODED_MAG_CONV_COEF;
}

bool Mpu_Init (void) {
    bool RetVal = false;
    if (Mpu_Detect()) {
        if(Mpu_Config()) {
            RetVal = true;
        }
    }
    return RetVal;
}

/* TODO: transfer this functionality from default task */
void HandleExt3IRQ (void) {
    vTaskNotifyGiveFromISR((TaskHandle_t)defaultTaskHandle, NULL);
}

/*void TestDataRequest (void) {
    sImuData_t ImuData;
    sImuRawData_t ImuRawData;
    if (Mpu_Detect()) {
        if(Mpu_Config()) {
            if (Mpu_ImuRead(&ImuRawData)) {
                Mpu_ConvertData(&ImuData, &ImuRawData);
                Mpu_PrintData(&ImuData);
            }
        } else {
            PrintToUart(eUart_1, "MPU config failed\r");
        }
    }
}*/

bool ReadIMU (sImuData_t *ImuData) {
    bool RetVal = false;
    sImuRawData_t ImuRawData;
    if (Mpu_ImuRead(&ImuRawData)) {
        Mpu_ConvertData(ImuData, &ImuRawData);
        Mpu_PrintData(ImuData);
        RetVal = true;
    }
    return RetVal;
}
