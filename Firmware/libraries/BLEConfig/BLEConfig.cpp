#include "BLEConfig.h"

/* Define Static Variables */
BLEConfig::prph_info_t BLEConfig::prphs[BLE_MAX_CONNECTION];
int BLEConfig::numberOfSensors;
uint8_t BLEConfig::connectionNum;
bool BLEConfig::isPeripheralDevice;
BLECharacteristic BLEConfig::quatCharacteristic[NUMBER_OF_UUIDS - 1];

const uint8_t *CUSTOM_CHARACTERSTIC_UUID[] =
{
    CUSTOM_UUID_1,
    CUSTOM_UUID_2,
    CUSTOM_UUID_3,
    CUSTOM_UUID_4,
    CUSTOM_UUID_5,
};

BLEConfig::BLEConfig(String nameOfDevice, int countPerip, int countCentral, bool peripheral)
{
    deviceName = nameOfDevice;
    peripheralCount = countPerip;
    centralCount = countCentral;
    isPeripheralDevice = peripheral;
    numberOfSensors = countPerip + countCentral;
}

void BLEConfig::init()
{
    /* Change the bandwidth for the peripheral and central devices connected */
    if(!isPeripheralDevice)
    {
        Bluefruit.configCentralBandwidth(BANDWIDTH_MAX);
    }
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

    /* Begin the Bluefruit specifying the number of peripheral and central connections */
    Bluefruit.begin(peripheralCount, centralCount);
    Bluefruit.setTxPower(4);

    /* Set the name of BLE Device */
    Bluefruit.setName(deviceName.c_str());

    /* Create callbacks for peripheral connection and disconnection */
    if(peripheralCount > 0)
    {
        Bluefruit.Periph.setConnectCallback(peripheralConnectCallback);
        Bluefruit.Periph.setDisconnectCallback(peripheralDisconnectCallback);
    }

    /* Create callbacks for central connection and disconnection */
    if(centralCount > 0)
    {
        Bluefruit.Central.setConnectCallback(centralConnectCallback);
        Bluefruit.Central.setDisconnectCallback(centralDisconnectCallback);
    }

    if(!isPeripheralDevice)
    {
        /* Create a pool of peripheral up to BLE_MAX_CONNECTION */
        initPeripheralPool();
        /* Start scanning for peripheral devices */
        startCentralScanning();
    }
    else
    {
        /* Create a UART connection */
        bleuartPeripheral.begin();
    }
}

void BLEConfig::initPeripheralPool()
{
    for (uint8_t idx=0; idx<BLE_MAX_CONNECTION; idx++)
    {
        // Invalid all connection handle
        prphs[idx].conn_handle = BLE_CONN_HANDLE_INVALID;

        // All of BLE Central Uart Serivce
        prphs[idx].ble_uart.begin();
        prphs[idx].ble_uart.setRxCallback(bleuartRxCallbackCentral);

        /* Clear the buffer to receive sensor data */
        memset(prphs[idx].sensorData, 0, sizeof(prphs[idx].sensorData));
    }
}

void BLEConfig::startCentralScanning()
{
    /* Start Central Scanning
     * - Enable auto scan if disconnected
     * - Interval = 100 ms, window = 80 ms
     * - Filter only accept bleuart service in advertising
     * - Don't use active scan (used to retrieve the optional scan response adv packet)
     * - Start(0) = will scan forever since no timeout is given
     */
    Bluefruit.Scanner.setRxCallback(scanCallbackCentral);
    Bluefruit.Scanner.restartOnDisconnect(true);
    Bluefruit.Scanner.setInterval(160, 80);             // in units of 0.625 ms
    Bluefruit.Scanner.filterUuid(BLEUART_UUID_SERVICE);
    Bluefruit.Scanner.useActiveScan(false);             // Don't request scan response data
    Bluefruit.Scanner.start(0);                         // 0 = Don't stop scanning after n seconds
}

void BLEConfig::setupBLEWearable()
{
    /* Create the BLE Service */
    createBLEService();

    /* Define the UUIDs given to the BLE Characteristics */
    createBLEUUIDs();

    /* Create the Characteristics based on the UUIDs */
    createBLECharacteristics();

    /* Configure the characteristic properties  */
    configureBLEProperties();
}

void BLEConfig::createBLEService()
{
    BLEUuid QUATERNION_SERVICE_UUID = BLEUuid(CUSTOM_UUID_0);
    quatService = BLEService(QUATERNION_SERVICE_UUID);
    quatService.begin();
}

void BLEConfig::createBLEUUIDs()
{
    for(int sensorNum = 0; sensorNum < numberOfSensors; sensorNum++)
    {
        charactersticUUIDs[sensorNum] = BLEUuid(CUSTOM_CHARACTERSTIC_UUID[sensorNum]);
    }
}

void BLEConfig::createBLECharacteristics()
{
    for(int sensorNum = 0; sensorNum < numberOfSensors; sensorNum++)
    {
        quatCharacteristic[sensorNum] = BLECharacteristic(charactersticUUIDs[sensorNum]);
    }
}

void BLEConfig::configureBLEProperties()
{
    /* Set the characteristic to use 8-bit values, with the sensor connected and detected */
    uint8_t hrmdata[2] = {0b00000110, 0x40};

    for(int sensorNum = 0; sensorNum < numberOfSensors; sensorNum++)
    {
        quatCharacteristic[sensorNum].setProperties(CHR_PROPS_NOTIFY);
        quatCharacteristic[sensorNum].setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
        quatCharacteristic[sensorNum].setMaxLen(48);
        quatCharacteristic[sensorNum].setCccdWriteCallback(cccdCallback);
        quatCharacteristic[sensorNum].begin();
        quatCharacteristic[sensorNum].write(hrmdata, 2);
    }
}

void BLEConfig::startAdvertising()
{
    /* Advertising packet */
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();

    if(!isPeripheralDevice)
    {
        /* Include Service UUID */
        Bluefruit.Advertising.addService(quatService);
    }
    else
    {
        /* Include UART Service */
        Bluefruit.Advertising.addService(bleuartPeripheral);
    }

    /* Include Name */
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
    Bluefruit.Advertising.setInterval(32, 244);       // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);         // number of seconds in fast mode
    Bluefruit.Advertising.start(0);                   // 0 = Don't stop advertising after n seconds
}

/**
 * Callback invoked when an connection is established
 * @param connHandle
 */
void BLEConfig::centralConnectCallback(uint16_t connHandle)
{
    // Find an available ID to use
    int id = findConnHandle(BLE_CONN_HANDLE_INVALID);

    // Eeek: Exceeded the number of connections !!!
    if ( id < 0 ) return;

    prph_info_t* peer = &prphs[id];
    peer->conn_handle = connHandle;

    Bluefruit.Connection(connHandle)->getPeerName(peer->peripheralName, sizeof(peer->peripheralName)-1);

    Serial.print("Connected to ");
    Serial.println(peer->peripheralName);

    Serial.print("Discovering BLE UART service ... ");

    if (peer->ble_uart.discover(connHandle))
    {
        Serial.println("Found it");
        Serial.println("Enabling TXD characteristic's CCCD notify bit");
        peer->ble_uart.enableTXD();

        Serial.println("Continue scanning for more peripherals");
        Bluefruit.Scanner.start(0);
    }
    else
    {
        Serial.println("Found ... NOTHING!");

        // disconnect since we couldn't find bleuart service
        Bluefruit.disconnect(connHandle);
    }

    connectionNum++;
}

/**
 * Callback invoked when a connection is dropped
 * @param connHandle
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void BLEConfig::centralDisconnectCallback(uint16_t connHandle, uint8_t reason)
{
    (void) connHandle;
    (void) reason;

    connectionNum--;

    // Mark the ID as invalid
    int id = findConnHandle(connHandle);

    // Non-existant connection, something went wrong, DBG !!!
    if ( id < 0 ) return;

    // Mark conn handle as invalid
    prphs[id].conn_handle = BLE_CONN_HANDLE_INVALID;

    Serial.print(prphs[id].peripheralName);
    Serial.println(" disconnected!");
}

/**
 * Callback invoked when BLE UART data is received
 * @param uartSvc Reference object to the service where the data
 * arrived.
 */
void BLEConfig::bleuartRxCallbackCentral(BLEClientUart& uartSvc)
{
    // uart_svc is prphs[conn_handle].bleuart
    uint16_t conn_handle = uartSvc.connHandle();

    int id = findConnHandle(conn_handle);
    prph_info_t* peer = &prphs[id];

    // Read then forward to all peripherals
    while ( uartSvc.available() )
    {
        memset(prphs[id].sensorData, 0, sizeof(prphs[id].sensorData)-1);
        uartSvc.read(prphs[id].sensorData,sizeof(prphs[id].sensorData)-1);
    }
}

/**
 * Callback invoked when scanner picks up an advertising packet
 * @param report Structural advertising data
 */
void BLEConfig::scanCallbackCentral(ble_gap_evt_adv_report_t* report)
{
    // Since we configure the scanner with filterUuid()
    // Scan callback only invoked for device with bleuart service advertised
    // Connect to the device with bleuart service in advertising packet
    Bluefruit.Central.connect(report);
}

/**
 * Find the connection handle in the peripheral array
 * @param connHhandle Connection handle
 * @return array index if found, otherwise -1
 */
int BLEConfig::findConnHandle(uint16_t connHhandle)
{
    for(int id=0; id<BLE_MAX_CONNECTION; id++)
    {
        if (connHhandle == prphs[id].conn_handle)
        {
            return id;
        }
    }

    return -1;
}

void BLEConfig::cccdCallback(uint16_t connHdl, BLECharacteristic* chr, uint16_t cccdValue)
{
    /* Display the raw request packet */
    Serial.print("CCCD Updated: ");
    Serial.print(cccdValue);
    Serial.println("");

    /*
     * Check the characteristic this CCCD update is associated with in case
     * this handler is used for multiple CCCD records.
    */
    for(int sensorNum = 0; sensorNum < numberOfSensors; sensorNum++)
    {
        if (chr->uuid == quatCharacteristic[sensorNum].uuid)
        {
            if (chr->notifyEnabled(connHdl))
            {
                Serial.print("Sensor ");
                Serial.print(sensorNum);
                Serial.println(" 'Notify' enabled");
            }
            else
            {
                Serial.print("Sensor ");
                Serial.print(sensorNum);
                Serial.println(" 'Notify' disabled");
            }
        }
    }
}

/**
 * Callback invoked when a connection happens peripheral
 * @param connHandle connection where this event happens
 */
void BLEConfig::peripheralConnectCallback(uint16_t connHandle)
{
    // Get the reference to current connection
    BLEConnection* connection = Bluefruit.Connection(connHandle);

    char central_name[32] = { 0 };
    connection->getPeerName(central_name, sizeof(central_name));

    Serial.print("Connected to ");
    Serial.println(central_name);

    /* request mtu exchange */
    if(isPeripheralDevice)
    {
        Serial.println("Request to change MTU");
        connection->requestMtuExchange(128);
    }

    /* delay a bit for all the request to complete */
    delay(1000);
}

/**
 * Callback invoked when a connection is dropped
 * @param connHandle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void BLEConfig::peripheralDisconnectCallback(uint16_t connHandle, uint8_t reason)
{
    (void) connHandle;
    (void) reason;

    Serial.println();
    Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

void BLEConfig::notifyNewValues()
{
    if (Bluefruit.Central.connected() && connectionNum == (numberOfSensors - 1))
    {
        for(int sensorNum = 0; sensorNum < numberOfSensors; sensorNum++)
        {
            quatCharacteristic[sensorNum].notify(prphs[sensorNum].sensorData);
        }
    }
}

void BLEConfig::uartPrintNewValues()
{
    if(Bluefruit.connected())
    {
        bleuartPeripheral.print(peripheralData);
    }
}
