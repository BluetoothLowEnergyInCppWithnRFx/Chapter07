#include "mbed.h"
#include "ble/BLE.h"

/** User interface I/O **/

// instantiate USB Serial
Serial serial(USBTX, USBRX);

// Status LED
DigitalOut statusLed(LED1, 0);

// Timer for blinking the statusLed
Ticker ticker;

/** Bluetooth Peripheral Properties **/

// Broadcast name
const static char BROADCAST_NAME[] = "MyDevice";

// Service UUID
static const uint16_t customServiceUuid  = 0x180C;

// array of all Service UUIDs
static const uint16_t uuid16_list[] = { customServiceUuid };

// Number of bytes in Characteristic
static const uint8_t characteristicLength = 20;

// Characteristic UUID
static const uint16_t characteristicUuid = 0x2A56;

/** State **/

// Storage for written Characteristic  value
char bleCharacteristicValue[characteristicLength];
uint16_t bleCharacteristicValueLength = 0;


/** Functions **/

/**
 * create a random string
 */
static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void writeRandomStringToCharacteristic();

/**
 * Callback triggered when the ble initialization process has finished
 *
 * @param[in] params Information about the initialized Peripheral
 */
void onBluetoothInitialized(BLE::InitializationCompleteCallbackContext *params);

/**
 * Callback handler when a Central has disconnected
 * 
 * @param[i] params Information about the connection
 */
void onCentralDisconnected(const Gap::DisconnectionCallbackParams_t *params);

/** Build Service and Characteristic Relationships **/

// Create a read/write/notify Characteristic
static uint8_t characteristicValue[characteristicLength] = {0};
ReadOnlyArrayGattCharacteristic<uint8_t, sizeof(characteristicValue)> characteristic(
    characteristicUuid, 
    characteristicValue);
 
// Bind Characteristics to Services
GattCharacteristic *characteristics[] = {&characteristic};
GattService        customService(customServiceUuid, characteristics, sizeof(characteristics) / sizeof(GattCharacteristic *));

/**
 * Main program and loop
 */
int main(void) {
    serial.baud(9600);
    serial.printf("Starting Peripheral\r\n");

    ticker.attach(writeRandomStringToCharacteristic, 5); // write random text every 5 seconds

    // initialized Bluetooth Radio
    BLE &ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(onBluetoothInitialized);

    
    // wait for Bluetooth Radio to be initialized
    while (ble.hasInitialized()  == false);

    while (1) {
        // save power when possible
        ble.waitForEvent();
    }
}

void writeRandomStringToCharacteristic() {
    statusLed = !statusLed;

    static const uint16_t stringLength = rand() % characteristicLength;
    uint8_t outputString[stringLength];
    for (int i=0; i < stringLength-1; i++) {
        outputString[i] = alphanum[rand() % alphanumLength];
    }
    outputString[stringLength-1] = '\0';
    
    BLE::Instance(BLE::DEFAULT_INSTANCE).gattServer().write(characteristic.getValueHandle(), outputString, stringLength);
}

void onBluetoothInitialized(BLE::InitializationCompleteCallbackContext *params) {
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    // quit if there's a problem
    if (error != BLE_ERROR_NONE) {
        return;
    }

    // Ensure that it is the default instance of BLE 
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    serial.printf("Describing Peripheral...");
    
    // attach Services
    ble.addService(customService);
 
    // process disconnections with a callback
    ble.gap().onDisconnection(onCentralDisconnected);

    // advertising parametirs
    ble.gap().accumulateAdvertisingPayload(
        GapAdvertisingData::BREDR_NOT_SUPPORTED |   // Device is Peripheral only
        GapAdvertisingData::LE_GENERAL_DISCOVERABLE); // always discoverable
    // broadcast name
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)BROADCAST_NAME, sizeof(BROADCAST_NAME));
    //  advertise services
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    // allow connections
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // advertise every 1000ms
    ble.gap().setAdvertisingInterval(1000); // 1000ms
    // begin advertising
    ble.gap().startAdvertising();

    serial.printf(" done\r\n");
}

void onCentralDisconnected(const Gap::DisconnectionCallbackParams_t *params) {
    BLE::Instance().gap().startAdvertising();
    serial.printf("Central disconnected\r\n");
}