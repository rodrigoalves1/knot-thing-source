/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <KNoTThing.h>

#define SS_PIN              6
#define SPEED_SENSOR_ID     3
#define SPEED_SENSOR_NAME   "Speed Sensor"

#define LED                 7
#define LIGHT_BULB_ID       1
#define LIGHT_BULB_NAME     "Light bulb"

#define POT_NAME     "Potentiometer"
#define POT_ID       2

KNoTThing thing;
static int32_t speed_value = 0;
static uint8_t led_value = 0;
static int32_t pot_value = 0;

static int speed_read(int32_t *val, int32_t *multiplier)
{

    *val = digitalRead(SS_PIN);
    *multiplier = 1;
    Serial.print("speed_read(): ");
    Serial.println(*val);
    return 0;
}

static int speed_write(int32_t *val, int32_t *multiplier)
{
    speed_value = *val;
    Serial.print("speed_write(): ");
    Serial.println(*val);
    return 0;
}

static int led_read(uint8_t *val)
{
    *val = led_value;
    Serial.print(*val);
    return 0;
}

static int led_write(uint8_t *val)
{
    led_value = *val;
    digitalWrite(LED, led_value);
    Serial.print(led_value);
    return 0;
}

static int pot_read(int32_t *val_int, uint32_t *val_dec, int32_t *multiplier)
{
    pot_value = analogRead(A5);
    float voltage = pot_value * (5.0 / 1023);
    *val_int = (int32_t) voltage;
    *val_dec = (uint32_t) (voltage - *val_int) * 10;
    Serial.print("POT: ");
    Serial.print(*val_int);
    Serial.print(".");
    Serial.println(*val_dec);
    *multiplier = 1;
    return 0;
}

static int pot_write(int32_t *val_int, uint32_t *val_dec, int32_t *multiplier)
{
    return 0;
}

void setup()
{
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    pinMode(SS_PIN, INPUT);
    thing.init("Speed");
    thing.registerIntData(SPEED_SENSOR_NAME, SPEED_SENSOR_ID, KNOT_TYPE_ID_SPEED, KNOT_UNIT_SPEED_MS, speed_read, speed_write);
    thing.registerBoolData(LIGHT_BULB_NAME, LIGHT_BULB_ID, KNOT_TYPE_ID_SWITCH, KNOT_UNIT_NOT_APPLICABLE, led_read, led_write);
    thing.registerFloatData(POT_NAME,POT_ID, KNOT_TYPE_ID_VOLTAGE, KNOT_UNIT_VOLTAGE_V, pot_read, pot_write);
}


void loop()
{
    thing.run();
}