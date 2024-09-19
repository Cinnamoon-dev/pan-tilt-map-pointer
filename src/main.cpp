#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "d013b1b9-1363-4eb1-8828-767c78631c27"
#define CHARACTERISTIC_UUID "be7a367f-ed56-40e7-aea7-272614708747"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool oldDeviceConnected = false;
bool deviceConnected = false;

char msg[20] = "Initial Message";

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if(value.length() > 0) {
            Serial.print("Received from APP: ");

            for(int i = 0; i < value.length(); i++) {
                Serial.print(value[i]);
            }

            Serial.println();
        }
    }
};

class MyServerCallbacks: public BLEServerCallbacks { 
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    BLEDevice::init("cleitinBLE");
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService* pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );

    pCharacteristic->addDescriptor(new BLEDescriptor("5dacf565-7489-4b7b-8bec-fc93e5e5182f"));
    pCharacteristic->setCallbacks(new MyCallbacks());
    
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}

void serialCommands() {
  if(Serial.available()) {
    String cmd = Serial.readString();
    
    cmd.toCharArray(msg, cmd.length());
    Serial.print("Writed to APP : ");
    Serial.println(msg);
    pCharacteristic->setValue(msg);
    pCharacteristic->notify();
  }
}

void loop() {
    serialCommands();
    
    // notify changed value
    if (deviceConnected) {
        pCharacteristic->setValue(msg);
        pCharacteristic->notify();
        delay(100); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        Serial.println("connected");
        oldDeviceConnected = deviceConnected;
    }
}