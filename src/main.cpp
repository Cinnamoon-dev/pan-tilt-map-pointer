#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>

int machine_state = 0;

void stringFormatT(char* buffer, const char *format, ...) {
    va_list args;
    va_start(args, format);

    vsprintf(buffer, format, args);

    va_end(args);
}


// Servo
// ----------------------------------------------------------------------------------- //
#include <ESP32Servo.h>
// ----------------------------------------------------------------------------------- //

// Servo
// ----------------------------------------------------------------------------------- //
Servo servo_x_machine;
const int servo_x_pin = 18;

Servo servo_y_machine;
const int servo_y_pin = 19;

const int joystick_pin_x  = A0; // GPIO 36
const int joystick_pin_y  = A10; // GPIO 4
const int joystick_pin_sw = 33; // D33

int state_y_position = 0;
int state_x_position = 90;

int lower_limit_x = -1;
int lower_limit_y = -1;

int upper_limit_x = -1;
int upper_limit_y = -1;

int sw_count = 0;

int isSwPressed () {
    int value = analogRead(joystick_pin_sw);
    if(!value) {
        delay(1807);
    }

    value = analogRead(joystick_pin_sw);

    return value;
}

int isLowerLimitSet() {
    return lower_limit_x != -1 && lower_limit_y != -1;
}

int isUpperLimitSet() {
    return upper_limit_x != -1 && upper_limit_y != -1;
}

void moveJoystick() {
    int map_volts_x_value = analogRead(joystick_pin_x);
    int map_volts_y_value = analogRead(joystick_pin_y);

    if (map_volts_x_value < 200 && state_x_position < 180){
        state_x_position++;
    }
    if (map_volts_x_value > 3895 && state_x_position > 0){
        state_x_position--;
    }

    if (map_volts_y_value < 200 && state_y_position < 90){
        state_y_position++;
    }
    if (map_volts_y_value > 3895 && state_y_position > 0){
        state_y_position--;
    }

    //Serial.printf("X : %d \n", analogRead(joystick_pin_x));
    //Serial.printf("Y : %d \n", analogRead(joystick_pin_y));
    servo_x_machine.write(state_x_position);
    servo_y_machine.write(state_y_position);
    Serial.printf("X : %d \n", state_x_position);
    Serial.printf("Y : %d \n", state_y_position);

    delay(15);
}

void moveJoystickSquare() {
    int map_volts_x_value = analogRead(joystick_pin_x);
    int map_volts_y_value = analogRead(joystick_pin_y);

    if (map_volts_x_value < 200 && state_x_position < upper_limit_x){
        state_x_position++;
    }
    if (map_volts_x_value > 3895 && state_x_position > lower_limit_x){
        state_x_position--;
    }

    if (map_volts_y_value < 200 && state_y_position < upper_limit_y){
        state_y_position++;
    }
    if (map_volts_y_value > 3895 && state_y_position > lower_limit_y){
        state_y_position--;
    }

    //Serial.printf("X : %d \n", analogRead(joystick_pin_x));
    //Serial.printf("Y : %d \n", analogRead(joystick_pin_y));
    servo_x_machine.write(state_x_position);
    servo_y_machine.write(state_y_position);
    Serial.printf("LimitX : %d-%d \n", lower_limit_x, upper_limit_x);
    Serial.printf("LimitY : %d-%d \n", lower_limit_y, upper_limit_y);
    Serial.printf("X : %d \n", state_x_position);
    Serial.printf("Y : %d \n", state_y_position);

    delay(15);
}
// ----------------------------------------------------------------------------------- //

// BLE
// ----------------------------------------------------------------------------------- //
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "d013b1b9-1363-4eb1-8828-767c78631c27"
#define CHARACTERISTIC_UUID "be7a367f-ed56-40e7-aea7-272614708747"

// ----------------------------------------------------------------------------------- //

// BLE
// ----------------------------------------------------------------------------------- //
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool oldDeviceConnected = false;
bool deviceConnected = false;

char msg[20] = "";

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if(value.length() > 0) {
            Serial.print("Received from APP: ");

            for(int i = 0; i < value.length(); i++) {
                Serial.print(value[i]);
            }

            Serial.println();

            if (machine_state == 0) {
                if (value.compare("a") == 0) {
                    lower_limit_x = state_x_position;
                    lower_limit_y = state_y_position;

                    pCharacteristic->setValue("lower");
                    pCharacteristic->notify();
                }

                if (value.compare("b") == 0) {
                    upper_limit_x = state_x_position;
                    upper_limit_y = state_y_position;

                    pCharacteristic->setValue("upper");
                    pCharacteristic->notify();

                    machine_state = 1;
                }

                Serial.printf("%d,%dX%d,%d\n", lower_limit_x, lower_limit_y, upper_limit_x, upper_limit_y);
            }

            if (machine_state == 1) {
                Serial.printf("second state\n");
            }
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

// Unused
// Look for MyCallback class
void serialCommands() {
  if(Serial.available()) {
    String cmd = Serial.readString();
    char coordinates[20];
    
    cmd.toCharArray(msg, cmd.length());
    Serial.print("Received from app: ");
    Serial.println(msg);

    // set lower if not
    // set upper if lower setted
    if(!isLowerLimitSet()) {
        lower_limit_x = state_x_position;
        lower_limit_y = state_y_position;
    }

    if(!isUpperLimitSet()) {
        upper_limit_x = state_x_position;
        upper_limit_y = state_y_position;
    }

    stringFormatT(msg, "%d,%dX%d,%d", lower_limit_x, lower_limit_y, upper_limit_x, upper_limit_y);
    Serial.println(msg);
    pCharacteristic->setValue(msg);
    pCharacteristic->notify();
  }
}

void bleConn() {
    // notify changed value
    if (deviceConnected) {
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
// ----------------------------------------------------------------------------------- //

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

    // Servo
    // ----------------------------------------------------------------------------------- //
    servo_x_machine.attach(servo_x_pin);
    servo_x_machine.write(state_x_position);

    servo_y_machine.attach(servo_y_pin);
    servo_y_machine.write(state_y_position);
    // ----------------------------------------------------------------------------------- //
}

void loop() {
    if(machine_state == 0) {
        bleConn();
        moveJoystick();
    }

    if(machine_state == 1) {
        bleConn();
        moveJoystickSquare();
    }
}