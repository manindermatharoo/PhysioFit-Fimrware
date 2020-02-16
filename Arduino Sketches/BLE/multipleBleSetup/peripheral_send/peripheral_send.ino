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
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"

// BLE Service
BLEDfu  bledfu;    // OTA DFU service
BLEDis  bledis;    // device information
BLEUart bleuart;   // uart over ble
BLEBas  blebas;    // battery

/* IMU Setup */
BNO080 myIMU;
float quaternion[4];

void setup()
{
    Serial.begin(115200);
    while ( !Serial ) delay(10);     // for nrf52840 with native usb

    Serial.println("Bluefruit52 BLEUART Example");
    Serial.println("---------------------------\n");

    /* This configures a higher MTU size of 128 bytes */
    Bluefruit.configPrphBandwidth(BANDWIDTH_HIGH);

    Bluefruit.begin();
    //TODO: Tune this value (maximum range, minimum power)
    Bluefruit.setTxPower(4);        // Check bluefruit.h for supported values
    Bluefruit.setName("Peripheral 1");
    //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    // To be consistent OTA DFU should be added first if it exists
    bledfu.begin();
    Serial.println("Began OTA DFU Service");

    // Configure and Start Device Information Service
    bledis.setManufacturer("Adafruit Industries");
    bledis.setModel("Bluefruit Feather52");
    bledis.begin();
    Serial.println("Began Device Information Service");

    // Configure and Start BLE Uart Service
    bleuart.begin();
    Serial.println("Began BLE UART Service");

    // Start BLE Battery Service
    blebas.begin();
    blebas.write(100);
    Serial.println("Began BLE Battery Service");

    // Set up and start advertising
    startAdv();

    /* Set up the IMU to output the rotation vector */
    // Wire.begin();

    // if (myIMU.begin() == false)
    // {
    //     Serial.println("BNO080 not detected at default I2C address. Check your jumpers and the hookup guide. Freezing...");
    //     while(1);
    // }

    // Wire.setClock(400000); //Increase I2C data rate to 400kHz

    // myIMU.enableRotationVector(10); //Send data update every 10ms

    // Serial.println(F("Rotation vector enabled"));
}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include bleuart 128-bit uuid
    Bluefruit.Advertising.addService(bleuart);

    // Secondary Scan Response packet (optional)
    // Since there is no room for 'Name' in Advertising packet
    Bluefruit.ScanResponse.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval: fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);       // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);         // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                   // 0 = Don't stop advertising after n seconds
}

void loop()
{
    // if (myIMU.dataAvailable() == true)
    // {
        quaternion[0] = 0.99;
        quaternion[1] = -0.12;
        quaternion[2] = 0.42;
        quaternion[3] = 0.73;

        String temp = "";
        temp += quaternion[0];
        temp += ",";
        temp += quaternion[1];
        temp += ",";
        temp += quaternion[2];
        temp += ",";
        temp += quaternion[3];
        temp += ",";

        bleuart.print(temp);
    // }
}

/**
 * Callback invoked when a connection happens
 * @param conn_handle connection where this event happens
 */
void connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);

    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));

    Serial.print("Connected to ");
    Serial.println(central_name);

    /* request mtu exchange */
    Serial.println("Request to change MTU");
    connection->requestMtuExchange(128);

    /* delay a bit for all the request to complete */
    delay(1000);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;

    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}
