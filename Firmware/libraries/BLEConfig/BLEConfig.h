#ifndef WEARABLE_BLE_H
#define WEARABLE_BLE_H

#include <bluefruit.h>
#include <Arduino.h>
#include "wearable_uuid.h"

/* Create an array of all the custom UUIDs available for Characteristics */
extern const uint8_t *CUSTOM_CHARACTERSTIC_UUID[];

class BLEConfig
{
    public:
        /* Struct containing peripheral info */
        /* Only to be used by the central device */
        typedef struct
        {
            char peripheralName[20];   // Name of the peripheral connected to
            uint16_t conn_handle;       // Connection handle
            BLEClientUart ble_uart;     // BLE Client UART from central to peripheral
            char sensorData[48];       // Buffer containing the received sensor data
        } prph_info_t;

        static prph_info_t prphs[BLE_MAX_CONNECTION];

        static int numberOfSensors;

        char peripheralData[48];

        BLEConfig(String nameOfDevice, int countPerip, int countCentral, bool peripheral);

        void init();
        void setupBLEWearable();
        void startAdvertising();

        /* Functions used to send data */
        void notifyNewValues();
        void uartPrintNewValues();

    protected:
    private:
        String deviceName;
        int peripheralCount, centralCount;
        static uint8_t connectionNum;
        static bool isPeripheralDevice;

        /* Initializtion of Central Device */
        void initPeripheralPool();
        void startCentralScanning();

        /* Only to be used by the peripherals not central device */
        BLEUart bleuartPeripheral;

        BLEService quatService;
        BLEUuid charactersticUUIDs[NUMBER_OF_UUIDS - 1];
        static BLECharacteristic quatCharacteristic[NUMBER_OF_UUIDS - 1];

        /* Set up the BLE Services and Charactersitics */
        void createBLEService();
        void createBLEUUIDs();
        void createBLECharacteristics();
        void configureBLEProperties();

        /* Callbacks for Central Connection */
        static void centralConnectCallback(uint16_t connHandle);
        static void centralDisconnectCallback(uint16_t connHandle, uint8_t reason);
        static void bleuartRxCallbackCentral(BLEClientUart& uartSvc);
        static void scanCallbackCentral(ble_gap_evt_adv_report_t* report);
        static void cccdCallback(uint16_t connHdl, BLECharacteristic* chr, uint16_t cccdValue);
        static int findConnHandle(uint16_t connHhandle);

        /* Callbacks for Peripheral Connection */
        static void peripheralConnectCallback(uint16_t connHandle);
        static void peripheralDisconnectCallback(uint16_t connHandle, uint8_t reason);
};

#endif // WEARABLE_BLE_H
