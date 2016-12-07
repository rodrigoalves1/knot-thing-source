/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <KNoTThing.h>

#define SPEED_SENSOR_ID     3
#define SPEED_SENSOR_NAME   "Speed Sensor"

#define LED                 7
#define LIGHT_BULB_ID       1
#define LIGHT_BULB_NAME     "Light bulb"


KNoTThing thing;
static int32_t speed_value = 0;
static uint8_t led_value = 0;

static int speed_read(int32_t *val, int32_t *multiplier)
{

    *val = speed_value++;
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

void setup()
{
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    thing.init("Speed");
    thing.registerIntData(SPEED_SENSOR_NAME, SPEED_SENSOR_ID, KNOT_TYPE_ID_SPEED, KNOT_UNIT_SPEED_MS, speed_read, speed_write);
    thing.registerBoolData(LIGHT_BULB_NAME, LIGHT_BULB_ID, KNOT_TYPE_ID_SWITCH, KNOT_UNIT_NOT_APPLICABLE, led_read, led_write);
}

void loop()
{
    thing.run();
}

