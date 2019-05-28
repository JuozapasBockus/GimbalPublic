#ifndef _MPU9250_API_
#define _MPU9250_API_

#include <stdbool.h>
#include <stdint.h>


typedef struct {
    float X;
    float Y;
    float Z;
} sData3D_t;

typedef struct {
    sData3D_t A;
    sData3D_t G;
    sData3D_t M;
} sImuData_t;

bool Mpu_Init (void);
void HandleExt3IRQ (void);
bool ReadIMU (sImuData_t *ImuData);
void Mpu_PrintData (sImuData_t *ImuData);

#endif /* _MPU9250_API_ */
