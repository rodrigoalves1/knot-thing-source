/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "knot_thing_config.h"
#include "knot_types.h"
#include "knot_thing_main.h"

// TODO: normalize all returning error codes

#define KNOT_THING_EMPTY_ITEM		"EMPTY ITEM"

/* Keeps track of max data_items index were there is a sensor/actuator stored */
static uint8_t max_sensor_id;

static struct {
	// schema values
	uint8_t			value_type;	// KNOT_VALUE_TYPE_* (int, float, bool, raw)
	uint8_t			unit;		// KNOT_UNIT_*
	uint16_t		type_id;	// KNOT_TYPE_ID_*
	const char		*name;		// App defined data item name
	// data values
	knot_value_types	last_data;
	uint8_t			*last_value_raw;
	// config values
	knot_config		config;	// Flags indicating when data will be sent
	// Data read/write functions
	knot_data_functions	functions;
} data_items[KNOT_THING_DATA_MAX];

static void reset_data_items(void)
{
	int8_t index = 0;
	max_sensor_id = 0;

	for (index = 0; index < KNOT_THING_DATA_MAX; index++)
	{
		data_items[index].name					= KNOT_THING_EMPTY_ITEM;
		data_items[index].type_id				= KNOT_TYPE_ID_INVALID;
		data_items[index].unit					= KNOT_UNIT_NOT_APPLICABLE;
		data_items[index].value_type				= KNOT_VALUE_TYPE_INVALID;
		data_items[index].config.event_flags			= KNOT_EVT_FLAG_UNREGISTERED;

		data_items[index].last_data.val_f.multiplier		= 1; // as "last_data" is a union, we need just to
		data_items[index].last_data.val_f.value_int		= 0; // set the "biggest" member
		data_items[index].last_data.val_f.value_dec		= 0;
		data_items[index].config.lower_limit.val_f.multiplier	= 1; // as "lower_limit" is a union, we need just to
		data_items[index].config.lower_limit.val_f.value_int	= 0; // set the "biggest" member
		data_items[index].config.lower_limit.val_f.value_dec	= 0;
		data_items[index].config.upper_limit.val_f.multiplier	= 1; // as "upper_limit" is a union, we need just to
		data_items[index].config.upper_limit.val_f.value_int	= 0; // set the "biggest" member
		data_items[index].config.upper_limit.val_f.value_dec	= 0;
		data_items[index].last_value_raw			= NULL;

		data_items[index].functions.int_f.read	= NULL; // as "functions" is a union, we need just to
		data_items[index].functions.int_f.write	= NULL; // set only one of its members
	}
}

int data_function_is_valid(knot_data_functions *func)
{
	if (func == NULL)
		return -1;

	if (func->int_f.read == NULL && func->int_f.write == NULL)
		return -1;

	return 0;
}

uint8_t item_is_unregistered(uint8_t sensor_id)
{
	return (!(data_items[sensor_id].config.event_flags & KNOT_EVT_FLAG_UNREGISTERED));
}

int8_t knot_thing_init(const int protocol, const char *thing_name)
{
	reset_data_items();
	return 0;
}

void knot_thing_exit(void)
{

}

int8_t knot_thing_register_raw_data_item(uint8_t sensor_id, const char *name,
	uint8_t *raw_buffer, uint8_t raw_buffer_len, uint16_t type_id,
	uint8_t value_type, uint8_t unit, knot_data_functions *func)
{
	if (raw_buffer == NULL)
		return -1;

	if (raw_buffer_len != KNOT_DATA_RAW_SIZE)
		return -1;

	if (knot_thing_register_data_item(sensor_id, name, type_id, value_type,
		unit, func) != 0)
		return -1;

	data_items[sensor_id].last_value_raw	= raw_buffer;

	return 0;
}


int8_t knot_thing_register_data_item(uint8_t sensor_id, const char *name,
	uint16_t type_id, uint8_t value_type, uint8_t unit,
	knot_data_functions *func)
{
	if (sensor_id >= KNOT_THING_DATA_MAX)
		return -1;

	if ((item_is_unregistered(sensor_id) != 0) ||
		(knot_schema_is_valid(type_id, value_type, unit) != 0) ||
		name == NULL || (data_function_is_valid(func) != 0))
		return -1;

	data_items[sensor_id].name					= name;
	data_items[sensor_id].type_id					= type_id;
	data_items[sensor_id].unit					= unit;
	data_items[sensor_id].value_type				= value_type;
	// TODO: load flags and limits from persistent storage
	data_items[sensor_id].config.event_flags			= KNOT_EVT_FLAG_NONE; // remove KNOT_EVT_FLAG_UNREGISTERED flag
	data_items[sensor_id].last_data.val_f.multiplier		= 1; // as "last_data" is a union, we need just to
	data_items[sensor_id].last_data.val_f.value_int			= 0; // set the "biggest" member
	data_items[sensor_id].last_data.val_f.value_dec			= 0;
	data_items[sensor_id].config.lower_limit.val_f.multiplier	= 1; // as "lower_limit" is a union, we need just to
	data_items[sensor_id].config.lower_limit.val_f.value_int	= 0; // set the "biggest" member
	data_items[sensor_id].config.lower_limit.val_f.value_dec	= 0;
	data_items[sensor_id].config.upper_limit.val_f.multiplier	= 1; // as "upper_limit" is a union, we need just to
	data_items[sensor_id].config.upper_limit.val_f.value_int	= 0; // set the "biggest" member
	data_items[sensor_id].config.upper_limit.val_f.value_dec	= 0;
	data_items[sensor_id].last_value_raw				= NULL;

	data_items[sensor_id].functions.int_f.read			= func->int_f.read; // as "functions" is a union, we need just to
	data_items[sensor_id].functions.int_f.write			= func->int_f.write; // set only one of its members

	if (sensor_id > max_sensor_id)
		max_sensor_id = sensor_id;

	return 0;
}

int8_t knot_thing_config_data_item(uint8_t sensor_id, uint8_t event_flags,
	knot_value_types *lower_limit, knot_value_types *upper_limit)
{
	if ((sensor_id >= KNOT_THING_DATA_MAX) || item_is_unregistered(sensor_id) == 0)
		return -1;

	data_items[sensor_id].config.event_flags = event_flags;
	if (lower_limit != NULL)
	{
		data_items[sensor_id].config.lower_limit.val_f.multiplier	= lower_limit->val_f.multiplier; // as "lower_limit" is a union, we need just to
		data_items[sensor_id].config.lower_limit.val_f.value_int	= lower_limit->val_f.value_int;  // set the "biggest" member
		data_items[sensor_id].config.lower_limit.val_f.value_dec	= lower_limit->val_f.value_dec;
	}

	if (upper_limit != NULL)
	{
		data_items[sensor_id].config.upper_limit.val_f.multiplier	= upper_limit->val_f.multiplier; // as "upper_limit" is a union, we need just to
		data_items[sensor_id].config.upper_limit.val_f.value_int	= upper_limit->val_f.value_int;  // set the "biggest" member
		data_items[sensor_id].config.upper_limit.val_f.value_dec	= upper_limit->val_f.value_dec;
	}
	// TODO: store flags and limits on persistent storage
	return 0;
}

int knot_thing_create_schema(uint8_t i, knot_msg_schema *msg)
{
	knot_msg_schema entry;

	memset(&entry, 0, sizeof(entry));

	msg->hdr.type = KNOT_MSG_SCHEMA;

	if (data_items[i].name == KNOT_THING_EMPTY_ITEM)
		return -1;

	msg->sensor_id = i;
	entry.values.value_type = data_items[i].value_type;
	entry.values.unit = data_items[i].unit;
	entry.values.type_id = data_items[i].type_id;
	strncpy(entry.values.name, data_items[i].name,
						sizeof(entry.values.name));

	msg->hdr.payload_len = sizeof(entry.values) + sizeof(entry.sensor_id);

	memcpy(&msg->values, &entry.values, sizeof(msg->values));
	/*
	 * Every time a data item is registered we must update the max
	 * number of sensor_id so we know when schema ends;
	 */
	if (i == max_sensor_id)
		msg->hdr.type = KNOT_MSG_SCHEMA | KNOT_MSG_SCHEMA_FLAG_END;

	return 0;
}

static int data_item_read(uint8_t sensor_id, knot_data *data)
{
	uint8_t uint8_val = 0, uint8_buffer[KNOT_DATA_RAW_SIZE];
	int32_t int32_val = 0, multiplier = 0;
	uint32_t uint32_val = 0;

	if (data_items[sensor_id].name != KNOT_THING_EMPTY_ITEM) {
		switch (data_items[sensor_id].value_type) {
		case KNOT_VALUE_TYPE_RAW:
			if (data_items[sensor_id].functions.raw_f.read == NULL)
				return -1;
			if (data_items[sensor_id].functions.raw_f.read(&uint8_val, uint8_buffer) < 0)
				return -1;

			memcpy(data->raw, uint8_buffer, sizeof(data->raw));
			break;
		case KNOT_VALUE_TYPE_BOOL:
			if (data_items[sensor_id].functions.bool_f.read == NULL)
				return -1;
			if (data_items[sensor_id].functions.bool_f.read(&uint8_val) < 0)
				return -1;

			data->values.val_b = uint8_val;
			break;
		case KNOT_VALUE_TYPE_INT:
			if (data_items[sensor_id].functions.int_f.read == NULL)
				return -1;
			if (data_items[sensor_id].functions.int_f.read(&int32_val, &multiplier) < 0)
				return -1;

			data->values.val_i.value = int32_val;
			data->values.val_i.multiplier = multiplier;
			break;
		case KNOT_VALUE_TYPE_FLOAT:
			if (data_items[sensor_id].functions.float_f.read == NULL)
				return -1;

			if (data_items[sensor_id].functions.float_f.read(&int32_val, &uint32_val, &multiplier) < 0)
				return -1;

			data->values.val_f.value_int = int32_val;
			data->values.val_f.value_dec = uint32_val;
			data->values.val_f.multiplier = multiplier;
			break;
		default:
			return -1;
			break;
		}
	} else {
		return -1;
	}

	return 0;
}

static int data_item_write(uint8_t sensor_id, knot_data *data)
{
	uint8_t len;
	if (data_items[sensor_id].name != KNOT_THING_EMPTY_ITEM) {
		switch (data_items[sensor_id].value_type) {
		case KNOT_VALUE_TYPE_RAW:
			len = sizeof(data->raw);
			if (data_items[sensor_id].functions.raw_f.write == NULL)
				return -1;
			if (data_items[sensor_id].functions.raw_f.write(data->raw, &len) < 0)
				return -1;

			break;
		case KNOT_VALUE_TYPE_BOOL:
			if (data_items[sensor_id].functions.bool_f.write == NULL)
				return -1;
			if (data_items[sensor_id].functions.bool_f.write(&data->values.val_b) < 0)
				return -1;
			break;
		case KNOT_VALUE_TYPE_INT:
			if (data_items[sensor_id].functions.int_f.read == NULL)
				return -1;
			if (data_items[sensor_id].functions.int_f.write(&data->values.val_i.value, &data->values.val_i.multiplier) < 0)
				return -1;
			break;
		case KNOT_VALUE_TYPE_FLOAT:
			if (data_items[sensor_id].functions.float_f.write == NULL)
				return -1;

			if (data_items[sensor_id].functions.float_f.write(&data->values.val_f.value_int, &data->values.val_f.value_dec, &data->values.val_f.multiplier) < 0)
				return -1;
			break;
		default:
			return-1;
			break;
		}
	} else {
		return -1;
	}

	return 0;
}

int8_t knot_thing_run(void)
{
	uint8_t i = 0, uint8_val = 0, comparison = 0, uint8_buffer[KNOT_DATA_RAW_SIZE];
	int32_t int32_val = 0, multiplier = 0;
	uint32_t uint32_val = 0;
	/* TODO: Monitor messages from network */

	/*
	 * For all registered data items: verify if value
	 * changed according to the events registered.
	 */

	// TODO: add timer events
	for (i = 0; i < KNOT_THING_DATA_MAX; i++)
	{
		if (data_items[i].value_type == KNOT_VALUE_TYPE_RAW)
		{
			// Raw supports only KNOT_EVT_FLAG_CHANGE
			if ((KNOT_EVT_FLAG_CHANGE & data_items[i].config.event_flags) == 0)
				continue;

			if (data_items[i].functions.raw_f.read == NULL)
				continue;

			if (data_items[i].last_value_raw == NULL)
				continue;

			if (data_items[i].functions.raw_f.read(&uint8_val, uint8_buffer) < 0)
				continue;

			if (uint8_val != KNOT_DATA_RAW_SIZE)
				continue;

			if (memcmp(data_items[i].last_value_raw, uint8_buffer, KNOT_DATA_RAW_SIZE) == 0)
				continue;

			memcpy(data_items[i].last_value_raw, uint8_buffer, KNOT_DATA_RAW_SIZE);
			// TODO: Send message (as raw is not in last_data structure)
			continue; // Raw actions end here
		}
		else if (data_items[i].value_type == KNOT_VALUE_TYPE_BOOL)
		{
			if (data_items[i].functions.bool_f.read == NULL)
				continue;

			if (data_items[i].functions.bool_f.read(&uint8_val) < 0)
				continue;

			if (uint8_val != data_items[i].last_data.val_b)
			{
				comparison |= (KNOT_EVT_FLAG_CHANGE & data_items[i].config.event_flags);
				data_items[i].last_data.val_b = uint8_val;
			}

		}
		else if (data_items[i].value_type == KNOT_VALUE_TYPE_INT)
		{
			if (data_items[i].functions.int_f.read == NULL)
				continue;

			if (data_items[i].functions.int_f.read(&int32_val, &multiplier) < 0)
				continue;

			// TODO: add multiplier to comparison
			if (int32_val < data_items[i].config.lower_limit.val_i.value)
				comparison |= (KNOT_EVT_FLAG_LOWER_THRESHOLD & data_items[i].config.event_flags);
			else if (int32_val > data_items[i].config.upper_limit.val_i.value)
				comparison |= (KNOT_EVT_FLAG_UPPER_THRESHOLD & data_items[i].config.event_flags);
			if (int32_val != data_items[i].last_data.val_i.value)
				comparison |= (KNOT_EVT_FLAG_CHANGE & data_items[i].config.event_flags);

			data_items[i].last_data.val_i.value = int32_val;
			data_items[i].last_data.val_i.multiplier = multiplier;
		}
		else if (data_items[i].value_type == KNOT_VALUE_TYPE_FLOAT)
		{
			if (data_items[i].functions.float_f.read == NULL)
				continue;

			if (data_items[i].functions.float_f.read(&int32_val, &uint32_val, &multiplier) < 0)
				continue;

			// TODO: add multiplier and decimal part to comparison
			if (int32_val < data_items[i].config.lower_limit.val_f.value_int)
				comparison |= (KNOT_EVT_FLAG_LOWER_THRESHOLD & data_items[i].config.event_flags);
			else if (int32_val > data_items[i].config.upper_limit.val_f.value_int)
				comparison |= (KNOT_EVT_FLAG_UPPER_THRESHOLD & data_items[i].config.event_flags);
			if (int32_val != data_items[i].last_data.val_f.value_int)
				comparison |= (KNOT_EVT_FLAG_CHANGE & data_items[i].config.event_flags);

			data_items[i].last_data.val_f.value_int = int32_val;
			data_items[i].last_data.val_f.value_dec = uint32_val;
			data_items[i].last_data.val_f.multiplier = multiplier;
		}
		else // This data item is not registered with a valid value type
		{
			continue;
		}

		// Nothing changed
		if (comparison == 0)
			continue;

		// TODO: If something changed, send message
	}

	return 0;
}
