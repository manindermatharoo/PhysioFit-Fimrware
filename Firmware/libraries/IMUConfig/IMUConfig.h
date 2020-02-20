#ifndef WEARABLE_IMU_H
#define WEARABLE_IMU_H

#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"

class IMUConfig
{
    public:
        IMUConfig();

        void init();
        void retreiveQuaternions(char* buffer);

    protected:
    private:
        BNO080 myIMU;
        float quaternion[4];
};

#endif /* WEARABLE_IMU_H */
