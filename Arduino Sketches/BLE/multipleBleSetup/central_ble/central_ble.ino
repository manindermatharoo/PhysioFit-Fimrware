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

/*
 * This sketch demonstrates the central API() that allows you to connect
 * to multiple peripherals boards (Bluefruit nRF52 in peripheral mode, or
 * any Bluefruit nRF51 boards).
 *
 * One or more Bluefruit boards, configured as a peripheral with the
 * bleuart service running are required for this demo.
 *
 * This sketch will:
 *    - Read data from the HW serial port (normally USB serial, accessible
 *        via the Serial Monitor for example), and send any incoming data to
 *        all other peripherals connected to the central device.
 *    - Forward any incoming bleuart messages from a peripheral to all of
 *        the other connected devices.
 *
 * It is recommended to give each peripheral board a distinct name in order
 * to more easily distinguish the individual devices.
 *
 * Connection Handle Explanation
 * -----------------------------
 * The total number of connections is BLE_MAX_CONNECTION (20)
 *
 * The 'connection handle' is an integer number assigned by the SoftDevice
 * (Nordic's proprietary BLE stack). Each connection will receive it's own
 * numeric 'handle' starting from 0 to BLE_MAX_CONNECTION-1, depending on the order
 * of connection(s).
 *
 * - E.g If our Central board connects to a mobile phone first (running as a peripheral),
 * then afterwards connects to another Bluefruit board running in peripheral mode, then
 * the connection handle of mobile phone is 0, and the handle for the Bluefruit
 * board is 1, and so on.
 */

/* LED PATTERNS
 * ------------
 * LED_RED     - Blinks pattern changes based on the number of connections.
 * LED_BLUE    - Blinks constantly when scanning
 */

#include <bluefruit.h>

#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"

#define NUM_OF_SENSORS 2
#define IMU_CONNECTED

const uint8_t CUSTOM_UUID_0[] =
{
    0x4B, 0x91, 0x31, 0xC3, 0xC9, 0xC5, 0xCC, 0x8F,
    0x9E, 0x45, 0xB5, 0x1F, 0x01, 0xC2, 0xAF, 0x4F
};
//  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

const uint8_t CUSTOM_UUID_1[] =
{
    0xA0, 0xDB, 0xD3, 0x6A, 0x00, 0xA6, 0xF7, 0x8C,
    0xE7, 0x11, 0x8F, 0x71, 0x1A, 0xFF, 0x67, 0xDF
};
// df67ff1a-718f-11e7-8cf7-a6006ad3dba0

const uint8_t CUSTOM_UUID_2[] =
{
    0x59, 0x6E, 0x65, 0x3A, 0x5E, 0x13, 0xBD, 0x98,
    0x58, 0x4D, 0xD9, 0x8A, 0x38, 0xDB, 0xF7, 0x58
};
// 58f7db38-8ad9-4d58-98bd-135e3a656e59

const uint8_t CUSTOM_UUID_3[] =
{
    0x9D, 0x40, 0xE3, 0xD2, 0xB2, 0x1E, 0x2F, 0x95,
    0x81, 0x40, 0xFF, 0xF2, 0x9F, 0xB7, 0x7D, 0x67
};
// 677db79f-f2ff-4081-952f-1eb2d2e3409d

const uint8_t CUSTOM_UUID_4[] =
{
    0x0A, 0xE1, 0xBB, 0x78, 0x18, 0x67, 0x62, 0x98,
    0x61, 0x40, 0xE0, 0xBB, 0x5A, 0xB6, 0xC8, 0xEF
};
// efc8b65a-bbe0-4061-9862-671878bbe10a

const uint8_t CUSTOM_UUID_5[] =
{
    0xD1, 0x7D, 0xD0, 0x5F, 0xB7, 0xB8, 0xEF, 0xAF,
    0x9A, 0x46, 0xFD, 0xD2, 0xE4, 0xDB, 0x9F, 0xEE
};
// ee9fdbe4-d2fd-469a-afef-b8b75fd07dd1

BLEUuid QUATERNION_SERVICE_UUID = BLEUuid(CUSTOM_UUID_0);
BLEUuid SENSOR_0_UUID = BLEUuid(CUSTOM_UUID_1);
BLEUuid SENSOR_1_UUID = BLEUuid(CUSTOM_UUID_2);
BLEUuid SENSOR_2_UUID = BLEUuid(CUSTOM_UUID_3);
BLEUuid SENSOR_3_UUID = BLEUuid(CUSTOM_UUID_4);
BLEUuid SENSOR_4_UUID = BLEUuid(CUSTOM_UUID_5);

BLEService        quaternion_service = BLEService(QUATERNION_SERVICE_UUID);
BLECharacteristic sensor0_char = BLECharacteristic(SENSOR_0_UUID);
BLECharacteristic sensor1_char = BLECharacteristic(SENSOR_1_UUID);
BLECharacteristic sensor2_char = BLECharacteristic(SENSOR_2_UUID);
BLECharacteristic sensor3_char = BLECharacteristic(SENSOR_3_UUID);
BLECharacteristic sensor4_char = BLECharacteristic(SENSOR_4_UUID);

// Peripheral uart service
BLEUart bleuart_peripheral;

// Struct containing peripheral info
typedef struct
{
    char name[16+1];

    uint16_t conn_handle;

    // Each prph need its own bleuart client service
    BLEClientUart bleuart;
} prph_info_t;

/* Struct containing the data for a peripheral */
typedef struct
{
    char buf[48];
    bool data_received;
} prph_data_t;

/* Peripheral info array (one per peripheral device)
 *
 * There are 'BLE_MAX_CONNECTION' central connections, but the
 * the connection handle can be numerically larger (for example if
 * the peripheral role is also used, such as connecting to a mobile
 * device). As such, we need to convert connection handles <-> the array
 * index where appropriate to prevent out of array accesses.
 *
 * Note: One can simply declares the array with BLE_MAX_CONNECTION and use connection
 * handle as index directly with the expense of SRAM.
 */
prph_info_t prphs[BLE_MAX_CONNECTION];

prph_data_t prph_data[NUM_OF_SENSORS];

char ble_data_send[48 * NUM_OF_SENSORS];

// Software Timer for blinking the RED LED
SoftwareTimer blinkTimer;
uint8_t connection_num = 0; // for blink pattern

/* IMU Setup */
BNO080 myIMU;
float quaternion[4];

void setup()
{
    Serial.begin(115200);
    while ( !Serial ) delay(10);     // for nrf52840 with native usb

    /* Initialize peripheral data */
    initalize_prph_data();

    // Initialize blinkTimer for 100 ms and start it
    blinkTimer.begin(100, blink_timer_callback);
    blinkTimer.start();

    Serial.println("Bluefruit52 Central Multi BLEUART Example");
    Serial.println("-----------------------------------------\n");

    // Config the peripheral connection with high bandwidth
    // Bluefruit.configPrphConn(35, BLE_GAP_EVENT_LENGTH_MIN, BLE_GATTS_HVN_TX_QUEUE_SIZE_DEFAULT, BLE_GATTC_WRITE_CMD_TX_QUEUE_SIZE_DEFAULT);
    Bluefruit.configCentralBandwidth(BANDWIDTH_MAX);
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

    // Initialize Bluefruit with max concurrent connections as Peripheral = 0, Central = 4
    // SRAM usage required by SoftDevice will increase with number of connections
    Bluefruit.begin(1, 1);
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

    // Set Name
    Bluefruit.setName("Bluefruit52-Central");

    // Callbacks for Peripheral
    Bluefruit.Periph.setConnectCallback(prph_connect_callback);
    Bluefruit.Periph.setDisconnectCallback(prph_disconnect_callback);

    // Callbacks for Central
    Bluefruit.Central.setConnectCallback(connect_callback_central);
    Bluefruit.Central.setDisconnectCallback(disconnect_callback_central);

    // Configure and Start BLE Uart Service for Peripheral
    // bleuart_peripheral.begin();

    // Init peripheral pool
    for (uint8_t idx=0; idx<BLE_MAX_CONNECTION; idx++)
    {
        // Invalid all connection handle
        prphs[idx].conn_handle = BLE_CONN_HANDLE_INVALID;

        // All of BLE Central Uart Serivce
        prphs[idx].bleuart.begin();
        prphs[idx].bleuart.setRxCallback(bleuart_rx_callback_central);
    }

    /* Start Central Scanning
     * - Enable auto scan if disconnected
     * - Interval = 100 ms, window = 80 ms
     * - Filter only accept bleuart service in advertising
     * - Don't use active scan (used to retrieve the optional scan response adv packet)
     * - Start(0) = will scan forever since no timeout is given
     */
    Bluefruit.Scanner.setRxCallback(scan_callback_central);
    Bluefruit.Scanner.restartOnDisconnect(true);
    Bluefruit.Scanner.setInterval(160, 80);             // in units of 0.625 ms
    Bluefruit.Scanner.filterUuid(BLEUART_UUID_SERVICE);
    Bluefruit.Scanner.useActiveScan(false);             // Don't request scan response data
    Bluefruit.Scanner.start(0);                         // 0 = Don't stop scanning after n seconds

    // Setup the Heart Rate Monitor service using
    // BLEService and BLECharacteristic classes
    Serial.println("Configuring the Heart Rate Monitor Service");
    setupHRM();

    // Setup the advertising packet(s)
    Serial.println("Setting up the advertising payload(s)");
    startAdv();

#ifdef IMU_CONNECTED
    /* Set up the IMU to output the rotation vector */
    Wire.begin();

    if (myIMU.begin() == false)
    {
        Serial.println("BNO080 not detected at default I2C address. Check your jumpers and the hookup guide. Freezing...");
        while(1);
    }

    delay(1000);

    Wire.setClock(400000); //Increase I2C data rate to 400kHz

    Wire.beginTransmission(0x4A);
    while (Wire.endTransmission() != 0);         //wait until device is responding (32 kHz XTO running)

    myIMU.enableRotationVector(100); //Send data update every 10ms

    Serial.println(F("Rotation vector enabled"));

#else
    Serial.println("IMU not connected");

#endif

}

/*
 * Initialize the peripheral data for each sensor
*/
void initalize_prph_data()
{
    for(int i = 0; i < NUM_OF_SENSORS; i++)
    {
        memset(prph_data[i].buf, 0, sizeof(prph_data[i].buf)-1);
        prph_data[i].data_received = false;
    }
}

/**
 * Callback invoked when scanner picks up an advertising packet
 * @param report Structural advertising data
 */
void scan_callback_central(ble_gap_evt_adv_report_t* report)
{
    // Since we configure the scanner with filterUuid()
    // Scan callback only invoked for device with bleuart service advertised
    // Connect to the device with bleuart service in advertising packet
    Bluefruit.Central.connect(report);
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback_central(uint16_t conn_handle)
{
    // Find an available ID to use
    int id = findConnHandle(BLE_CONN_HANDLE_INVALID);

    // Eeek: Exceeded the number of connections !!!
    if ( id < 0 ) return;

    prph_info_t* peer = &prphs[id];
    peer->conn_handle = conn_handle;

    Bluefruit.Connection(conn_handle)->getPeerName(peer->name, sizeof(peer->name)-1);

    Serial.print("Connected to ");
    Serial.println(peer->name);

    Serial.print("Discovering BLE UART service ... ");

    if ( peer->bleuart.discover(conn_handle) )
    {
        Serial.println("Found it");
        Serial.println("Enabling TXD characteristic's CCCD notify bit");
        peer->bleuart.enableTXD();

        Serial.println("Continue scanning for more peripherals");
        Bluefruit.Scanner.start(0);
    }
    else
    {
        Serial.println("Found ... NOTHING!");

        // disconnect since we couldn't find bleuart service
        Bluefruit.disconnect(conn_handle);
    }

    connection_num++;
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback_central(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;

    connection_num--;

    // Mark the ID as invalid
    int id = findConnHandle(conn_handle);

    // Non-existant connection, something went wrong, DBG !!!
    if ( id < 0 ) return;

    // Mark conn handle as invalid
    prphs[id].conn_handle = BLE_CONN_HANDLE_INVALID;

    Serial.print(prphs[id].name);
    Serial.println(" disconnected!");
}

/**
 * Callback invoked when BLE UART data is received
 * @param uart_svc Reference object to the service where the data
 * arrived.
 */
void bleuart_rx_callback_central(BLEClientUart& uart_svc)
{
    // uart_svc is prphs[conn_handle].bleuart
    uint16_t conn_handle = uart_svc.connHandle();

    int id = findConnHandle(conn_handle);
    prph_info_t* peer = &prphs[id];

    // Read then forward to all peripherals
    while ( uart_svc.available() )
    {
        memset(prph_data[id].buf, 0, sizeof(prph_data[id].buf)-1);
        uart_svc.read(prph_data[id].buf,sizeof(prph_data[id].buf)-1);
        // Serial.println(prph_data[id].buf);
        prph_data[id].data_received = true;
    }
}

void loop()
{
    char temp[48];

#ifdef IMU_CONNECTED
    if (myIMU.dataAvailable() == true)
    {
        quaternion[0] = myIMU.getQuatReal();
        quaternion[1] = myIMU.getQuatI();
        quaternion[2] = myIMU.getQuatJ();
        quaternion[3] = myIMU.getQuatK();
        Serial.println(quaternion[0]);
    }

#else
    quaternion[0] = 0.296997070312500;
    quaternion[1] = 0.250671386718750;
    quaternion[2] = 0.652587890625000;
    quaternion[3] = 0.650451660156250;

#endif

    // First check if we are connected to any peripherals
    if (Bluefruit.Central.connected() && connection_num == (NUM_OF_SENSORS - 1))
    {
        snprintf(temp, sizeof(temp), "%.8f,%.8f,%.8f,%.8f,", quaternion[0], quaternion[1],quaternion[2], quaternion[3]);

        memset(prph_data[NUM_OF_SENSORS-1].buf, 0, sizeof(prph_data[NUM_OF_SENSORS-1].buf));
        strcpy(prph_data[NUM_OF_SENSORS-1].buf, temp);

        sensor0_char.notify(prph_data[NUM_OF_SENSORS-1].buf);
        // sensor1_char.notify(prph_data[0].buf);
        // sensor2_char.notify(prph_data[NUM_OF_SENSORS-1].buf);
        // sensor3_char.notify(prph_data[0].buf);
        // sensor4_char.notify(prph_data[NUM_OF_SENSORS-1].buf);
    }
}

/**
 * Find the connection handle in the peripheral array
 * @param conn_handle Connection handle
 * @return array index if found, otherwise -1
 */
int findConnHandle(uint16_t conn_handle)
{
    for(int id=0; id<BLE_MAX_CONNECTION; id++)
    {
        if (conn_handle == prphs[id].conn_handle)
        {
            return id;
        }
    }

    return -1;
}

/**
 * Software Timer callback is invoked via a built-in FreeRTOS thread with
 * minimal stack size. Therefore it should be as simple as possible. If
 * a periodically heavy task is needed, please use Scheduler.startLoop() to
 * create a dedicated task for it.
 *
 * More information http://www.freertos.org/RTOS-software-timer.html
 */
void blink_timer_callback(TimerHandle_t xTimerID)
{
    (void) xTimerID;

    // Period of sequence is 10 times (1 second).
    // RED LED will toggle first 2*n times (on/off) and remain off for the rest of period
    // Where n = number of connection
    static uint8_t count = 0;

    // digitalToggle(LED_RED);

    if ( count < 2*connection_num ) digitalToggle(LED_RED);
    if ( count % 2 && digitalRead(LED_RED)) digitalWrite(LED_RED, LOW); // issue #98

    count++;
    if (count >= 10) count = 0;
}

/* ***********************************************************
Periperal
************************************************************ */
void setupHRM(void)
{
    quaternion_service.begin();

    sensor0_char.setProperties(CHR_PROPS_NOTIFY);
    sensor0_char.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    sensor0_char.setMaxLen(48);
    sensor0_char.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
    sensor0_char.begin();
    uint8_t hrmdata_0[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
    sensor0_char.write(hrmdata_0, 2);

    // sensor1_char.setProperties(CHR_PROPS_NOTIFY);
    // sensor1_char.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    // sensor1_char.setMaxLen(48);
    // sensor1_char.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
    // sensor1_char.begin();
    // uint8_t hrmdata_1[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
    // sensor1_char.write(hrmdata_1, 2);

    // sensor2_char.setProperties(CHR_PROPS_NOTIFY);
    // sensor2_char.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    // sensor2_char.setMaxLen(48);
    // sensor2_char.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
    // sensor2_char.begin();
    // uint8_t hrmdata_2[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
    // sensor2_char.write(hrmdata_2, 2);

    // sensor3_char.setProperties(CHR_PROPS_NOTIFY);
    // sensor3_char.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    // sensor3_char.setMaxLen(48);
    // sensor3_char.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
    // sensor3_char.begin();
    // uint8_t hrmdata_3[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
    // sensor3_char.write(hrmdata_3, 2);

    // sensor4_char.setProperties(CHR_PROPS_NOTIFY);
    // sensor4_char.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    // sensor4_char.setMaxLen(48);
    // sensor4_char.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
    // sensor4_char.begin();
    // uint8_t hrmdata_4[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
    // sensor4_char.write(hrmdata_4, 2);
}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    // Include HRM Service UUID
    Bluefruit.Advertising.addService(quaternion_service);

    // Include Name
    Bluefruit.Advertising.addName();

    /* Start Advertising
    * - Enable auto advertising if disconnected
    * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
    * - Timeout for fast mode is 30 seconds
    * - Start(timeout) with timeout = 0 will advertise forever (until connected)
    *
    * For recommended advertising interval
    * https://developer.apple.com/library/content/qa/qa1931/_index.html
    */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void prph_connect_callback(uint16_t conn_handle)
{
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(conn_handle);

    char peer_name[32] = { 0 };
    connection->getPeerName(peer_name, sizeof(peer_name));

    Serial.print("[Prph] Connected to ");
    Serial.println(peer_name);

    /* request mtu exchange */
    // Serial.println("Request to change MTU");
    // connection->requestMtuExchange(247);

    /* delay a bit for all the request to complete */
    delay(1000);
}

void prph_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    (void) conn_handle;
    (void) reason;

    Serial.println();
    Serial.println("[Prph] Disconnected");
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
    // Display the raw request packet
    Serial.print("CCCD Updated: ");
    //Serial.printBuffer(request->data, request->len);
    Serial.print(cccd_value);
    Serial.println("");

    // Check the characteristic this CCCD update is associated with in case
    // this handler is used for multiple CCCD records.
    if (chr->uuid == sensor0_char.uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("Sensor 0 'Notify' enabled");
        } else {
            Serial.println("Sensor 0 'Notify' disabled");
        }
    }

    if (chr->uuid == sensor1_char.uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("Sensor 1 'Notify' enabled");
        } else {
            Serial.println("Sensor 1 'Notify' disabled");
        }
    }

    if (chr->uuid == sensor2_char.uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("Sensor 2 'Notify' enabled");
        } else {
            Serial.println("Sensor 2 'Notify' disabled");
        }
    }

    if (chr->uuid == sensor3_char.uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("Sensor 3 'Notify' enabled");
        } else {
            Serial.println("Sensor 3 'Notify' disabled");
        }
    }

    if (chr->uuid == sensor4_char.uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("Sensor 4 'Notify' enabled");
        } else {
            Serial.println("Sensor 4 'Notify' disabled");
        }
    }
}
