// Aqui estão as lógicas que não serão mais usadas mas podem ser úteis no futuro
#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>

void stringFormatT(char* buffer, const char *format, ...) {
    va_list args;
    va_start(args, format);

    vsprintf(buffer, format, args);

    va_end(args);
}

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