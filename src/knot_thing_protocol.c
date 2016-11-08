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
#include <string.h>

#include "knot_thing_protocol.h"
#include "storage.h"
#include "comm.h"

/*KNoT client storage mapping */
#define KNOT_UUID_FLAG_ADDR		0
#define KNOT_UUID_FLAG_LEN		1
#define KNOT_UUID_ADDR			(KNOT_UUID_FLAG_ADDR + KNOT_UUID_FLAG_LEN)
#define KNOT_TOKEN_FLAG_ADDR		(KNOT_UUID_ADDR + KNOT_PROTOCOL_UUID_LEN)
#define KNOT_TOKEN_FLAG_LEN		1
#define KNOT_TOKEN_ADDR			(KNOT_TOKEN_FLAG_ADDR + KNOT_TOKEN_FLAG_LEN)

/* KNoT protocol client states */
#define STATE_DISCONNECTED		0
#define STATE_CONNECTING		1
#define STATE_AUTHENTICATING		2
#define STATE_REGISTERING		3
#define STATE_SCHEMA			4
#define STATE_ONLINE			5
#define STATE_ERROR			6
#define STATE_MAX			(STATE_ERROR+1)

static uint8_t enable_run = 0, schema_sensor_id = 0;
static const char *thingname;
static schema_function schemaf;
static data_function thing_read;
static data_function thing_write;
static config_function configf;

int knot_thing_protocol_init(uint8_t protocol, const char *thing_name,
					data_function read, data_function write,
				schema_function schema, config_function config)
{
	//TODO: open socket
	thingname = thing_name;
	enable_run = 1;
	schemaf = schema;
	thing_read = read;
	thing_write = write;
	config = config;
}

void knot_thing_protocol_exit(void)
{
	//TODO: close socket if needed
	enable_run = 0;
}

int knot_thing_protocol_run(void)
{
	static uint8_t state = STATE_DISCONNECTED;
	uint8_t uuid_flag = 0, token_flag = 0;
	char *uuid, *token;
	int retval = 0;
	size_t ilen;
	knot_msg *kreq

	if (enable_run == 0)
		return -1;

	/* Network message handling state machine */
	switch (state) {
	case STATE_DISCONNECTED:
		//TODO: call hal_comm_connect()
		state = STATE_CONNECTING;
	break;

	case STATE_CONNECTING:
		//TODO: verify connection status, if not connected,
		//	goto STATE_ERROR
		hal_storage_read(KNOT_UUID_FLAG_ADDR, &uuid_flag,
						KNOT_UUID_FLAG_LEN);
		hal_storage_read(KNOT_TOKEN_FLAG_ADDR, &token_flag,
						KNOT_TOKEN_FLAG_LEN);
		if(uuid_flag && token_flag) {
			hal_storage_read(KNOT_UUID_ADDR, uuid,
						KNOT_PROTOCOL_UUID_LEN);
			hal_storage_read(KNOT_TOKEN_ADDR, token,
					KNOT_PROTOCOL_TOKEN_LEN);

			if (send_auth() < 0)
				state = STATE_ERROR;
			else
				state = STATE_AUTHENTICATING;
		} else {
			if (send_register() < 0)
				state = STATE_ERROR;
			else
				state = STATE_REGISTERING;
		}
	break;

	case STATE_AUTHENTICATING:
		retval = read_auth();
		if (retval < 0)
			state = STATE_ERROR;
		else if (retval > 0)
			state = STATE_SCHEMA;
	break;

	case STATE_REGISTERING:
		retval = read_register();
		if (retval < 0)
			state = STATE_ERROR;
		else if (retval > 0)
			state = STATE_SCHEMA;
	break;

	case STATE_SCHEMA:
		/*Send schema individually*/
		retval = send_schema();
		schema_sensor_id++;
		if (retval < 0)
			state = STATE_ERROR;
		else (retval > 0)
			state = STATE_ONLINE;
	break;

	case STATE_ONLINE:
		ilen = hal_comm_recv(sockfd, buffer, sizeof(buffer));
		if (ilen > 0) {
			/* There is config or set data */
			kreq = buffer;
			switch (kreq->hdr.type) {
			case KNOT_MSG_CONFIG:
				config(kreq->config);
			case KNOT_MSG_SET_DATA:
				set_data(kreq->data);
			case KNOT_MSG_GET_DATA:
				/* TODO */
				break;
			default:
				/* Invalid command */
				break;
			}

		}
		//TODO: send messages according to the events
	break;

	case STATE_ERROR:
		//TODO: log error
		//TODO: close connection if needed
		//TODO: wait 1s
		state = STATE_DISCONNECTED;
	break;

	default:
		//TODO: log invalid state
		//TODO: close connection if needed
		state = STATE_DISCONNECTED;
	break;
	}

	return 0;
}

static int send_register(void)
{
	knot_msg_register msg;

	long int nbytes;
	int len = strlen(thingname);

	memset(&msg, 0, sizeof(msg));

	msg.hdr.type = KNOT_MSG_REGISTER_REQ;
	msg.hdr.payload_len = len;
	strncpy(msg.devName, thingname, len);
	/* FIXME: Open socket */
	nbytes = hal_comm_send(-1, &msg, sizeof(msg.hdr) + len);
	if (nbytes < 0) {
		return -1;
	}

	return 0;
}

static int read_register(void)
{
	int err = 0;
	long int nbytes;
	knot_msg_credential crdntl;

	memset(&crdntl, 0, sizeof(crdntl));
	/* FIXME: Open socket */
	nbytes = hal_comm_recv(-1, &crdntl, sizeof(crdntl));
	if (nbytes > 0) {
		if (crdntl.result != KNOT_SUCCESS) {
			err = -1;
			return err;
		}

		hal_storage_write(KNOT_UUID_ADDR, crdntl.uuid,
						KNOT_PROTOCOL_UUID_LEN);
		hal_storage_write(KNOT_TOKEN_ADDR, crdntl.token,
						KNOT_PROTOCOL_TOKEN_LEN);

		hal_storage_write(KNOT_UUID_FLAG_ADDR, 1, KNOT_UUID_FLAG_LEN);
		hal_storage_write(KNOT_TOKEN_FLAG_ADDR, 1, KNOT_TOKEN_FLAG_LEN);

		err = 1;
	}
	return err;
}

static int send_auth(void)
{
	knot_msg_authentication msg;
	knot_msg_result resp;
	ssize_t nbytes;

	memset(&msg, 0, sizeof(msg));

	msg.hdr.type = KNOT_MSG_AUTH_REQ;
	msg.hdr.payload_length = sizeof(msg.uuid) + sizeof(msg.token);

	strncpy(msg.uuid, uuid, sizeof(msg.uuid));
	strncpy(msg.token, token, sizeof(msg.token));

	nbytes = hal_comm_send(sock, &msg, sizeof(msg.hdr) +
							msg.hdr.payload_len);
	if (nbytes < 0)
		return -1;

	return 0;
}

static int read_auth(void)
{
	int err = 0;
	knot_msg_result resp;

	memset(&resp, 0, sizeof(resp));

	nbytes = hal_comm_recv(sock, &resp, sizeof(resp));
	if (nbytes > 0) {
		if (resp.result != KNOT_SUCCESS)
			return -1;
		err = 1;
	} else if (nbytes < 0) {
		err = -1;
	}

	return err;
}

static int send_schema(void)
{
	int err;
	knot_msg_schema *msg;
	ssize_t nbytes;

	err = schemaf(schema_sensor_id, msg);

	if (err < 0)
		return err;

	nbytes = hal_comm_send(sock, msg, sizeof(msg.hdr) +
						msg.hdr.payload_len);
	if (nbytes < 0)
		return -1;
	else if (nbytes > 0) {
		if (msg.hdr.type == KNOT_MSG_SCHEMA_FLAG_END) {
			schema_sensor_id = 0;
			return 1;
		}
	}

	return 0;

}

static int config(knot_msg_config config)
{
	int err;
	knot_msg_result resp;
	ssize_t nbytes;

	err = configf(config.sensor_id, config.values.event_flags,
			config.values.lower_limit, config.values.upper_limit);

	memset(&resp, 0, sizeof(resp));

	if (err < 0)
		resp.result = KNOT_ERROR_UNKNOWN;
	else
		resp.result = KNOT_SUCCESS;

	resp.hdr.type = KNOT_MSG_CONFIG_RESP;
	resp.hrd.payload_len = sizeof(resp.result);

	nbytes = hal_comm_send(sock, resp, sizeof(resp.hdr) + resp.result);
	if (nbytes < 0)
		return -1;
	else
		return 0;
}

static int set_data(knot_msg_data data)
{
	int err;

	err = thing_write(data.sensor_id, data.payload);

	return err;
}