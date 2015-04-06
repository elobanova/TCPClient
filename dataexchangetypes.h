/*
 * dataexchangetypes.h
 *
 *  Created on: Apr 3, 2015
 *      Author: ekaterina
 */

#ifndef DATAEXCHANGETYPES_H_
#define DATAEXCHANGETYPES_H_

#include <stdint.h>

#define JOKER_REQUEST_TYPE 1
#define JOKER_RESPONSE_TYPE 2

typedef struct joker_response {
	uint8_t type;
	uint32_t joke_length;
}__attribute__ ((__packed__)) joker_response;

typedef struct joker_request {
	uint8_t type;
	uint8_t len_first_name;
	uint8_t len_last_name;
}__attribute__ ((__packed__)) joker_request;

#endif /* DATAEXCHANGETYPES_H_ */
