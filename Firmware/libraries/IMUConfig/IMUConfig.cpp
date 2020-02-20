#include "IMUConfig.h"

IMUConfig::IMUConfig()
{}

void IMUConfig::init()
{
    /* Set up the IMU to output the rotation vector */
    Wire.begin();

    if (myIMU.begin() == false)
    {
        Serial.println("BNO080 not detected at default I2C address. Check your jumpers and the hookup guide. Freezing...");
        while(1);
    }

    Wire.setClock(400000); //Increase I2C data rate to 400kHz

    myIMU.enableRotationVector(10); //Send data update every 10ms
}

void IMUConfig::retreiveQuaternions(char* buffer)
{
    memset(buffer, 0, sizeof(buffer));
    if (myIMU.dataAvailable() == true)
    {
        quaternion[0] = myIMU.getQuatReal();
        quaternion[1] = myIMU.getQuatI();
        quaternion[2] = myIMU.getQuatJ();
        quaternion[3] = myIMU.getQuatK();
    }
    snprintf(buffer, sizeof(buffer), "%.8f,%.8f,%.8f,%.8f,", quaternion[0], quaternion[1],quaternion[2], quaternion[3]);
}
