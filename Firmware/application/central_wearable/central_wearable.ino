#include <BLEConfig.h>
#include <IMUConfig.h>

// #define IMU_CONNECTED

BLEConfig BLEConnection("Bluefruit-Central", 1, 1, false);

#ifdef IMU_CONNECTED
IMUConfig IMUConnection;
#endif

void setup()
{
    Serial.begin(115200);
    while ( !Serial ) delay(10);

    /* Initialize the BLE Connection */
    BLEConnection.init();
    Serial.println("Initialized the BLE Connection");

    /* Set up Wearable's Services and Characteristics */
    BLEConnection.setupBLEWearable();
    Serial.println("Configuring the Wearable Service & Characteristics");

    /* Setup the advertising packet(s) */
    BLEConnection.startAdvertising();
    Serial.println("Setting up the advertising payload(s)");

#ifdef IMU_CONNECTED
    /* Set up the IMU to output the rotation vector */
    IMUConnection.init();
    Serial.println(F("Rotation vector enabled"));

#else
    Serial.println("IMU not connected");

#endif
}

void loop()
{
#ifdef IMU_CONNECTED
    IMUConnection.retreiveQuaternions(BLEConnection.peripheralData);

#else
    /* Fabricate some Quaternion data to use */
    float quaternion[4];
    memset(BLEConnection.peripheralData, 0, sizeof(BLEConnection.peripheralData));
    quaternion[0] = 0.296997070312500;
    quaternion[1] = 0.250671386718750;
    quaternion[2] = 0.652587890625000;
    quaternion[3] = 0.650451660156250;
    snprintf(BLEConnection.peripheralData, sizeof(BLEConnection.peripheralData), "%.8f,%.8f,%.8f,%.8f,", quaternion[0], quaternion[1],quaternion[2], quaternion[3]);

#endif

    memset(BLEConnection.prphs[BLEConnection.numberOfSensors-1].sensorData, 0, sizeof(BLEConnection.prphs[BLEConnection.numberOfSensors-1].sensorData));
    strcpy(BLEConnection.prphs[BLEConnection.numberOfSensors-1].sensorData, BLEConnection.peripheralData);

    BLEConnection.notifyNewValues();
}
