/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/
#include <BLEConfig.h>
#include <IMUConfig.h>

// #define IMU_CONNECTED

BLEConfig BLEConnection("Peripheral-1", 1, 0, true);

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
    quaternion[0] = 0.239013671875000;
    quaternion[1] = -0.648071289062500;
    quaternion[2] = -0.274291992187500;
    quaternion[3] = -0.669067382812500;
    snprintf(BLEConnection.peripheralData, sizeof(BLEConnection.peripheralData), "%.8f,%.8f,%.8f,%.8f,", quaternion[0], quaternion[1],quaternion[2], quaternion[3]);
#endif

    BLEConnection.uartPrintNewValues();
}
