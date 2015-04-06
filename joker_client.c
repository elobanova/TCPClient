/*
 * joker_client.c
 *
 *  Created on: Apr 3, 2015
 *      Author: ekaterina
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "connectiontools.h"
#include "dataexchangetypes.h"

#define REQUIRED_NUMBER_OF_CMD_ARGUMENTS 3
#define NAME_MAX_LENGTH 20
#define MAX_BYTE_TO_SEND 128

int requestUserData(char msgtosend[MAX_BYTE_TO_SEND]) {
	char first_name[NAME_MAX_LENGTH];
	char last_name[NAME_MAX_LENGTH];
	int request_struct_size;
	struct request_header request_msg;

	//request the first name and last name
	printf("Please, enter your first name: ");
	scanf("%s", first_name);
	printf("Please, enter your last name: ");
	scanf("%s", last_name);

	uint8_t first_name_length = strlen(first_name);
	uint8_t second_name_length = strlen(last_name);
	request_msg.type = JOKER_REQUEST_TYPE;
	request_msg.len_first_name = first_name_length;
	request_msg.len_last_name = second_name_length;

	request_struct_size = sizeof(struct request_header);
	memcpy(msgtosend, (char *) &request_msg, request_struct_size);
	strcat(msgtosend, first_name);
	strcat(msgtosend, last_name);
	return request_struct_size + first_name_length + second_name_length;
}

int main(int argc, char *argv[]) {
	//connecion data
	int socketfd;
	int recv_bytes;
	int bytes_sent;
	int bytes_to_send;

	//exchange data
	char msgtosend[MAX_BYTE_TO_SEND];
	char buf[1000];

	if (argc != REQUIRED_NUMBER_OF_CMD_ARGUMENTS) {
		fprintf(stderr, "Hostname and port were not provided by the user.\n");
		return 1;
	}

	bytes_to_send = requestUserData(msgtosend);

	socketfd = setupsocket(argv);
	if (socketfd != -1) {
		bytes_sent = send(socketfd, msgtosend, bytes_to_send, 0);
		uint32_t len_of_joke = 0;
		int recv_bytes_sum = 0;
		//	do {
		recv_bytes = recvtimeout(socketfd, buf, sizeof buf);

		if (recv_bytes == ERROR_RESULT) {
			close(socketfd);
			fprintf(stderr, "Error during data receive.\n");
			return 1;
		} else if (recv_bytes == TIMEOUT_RESULT) {
			close(socketfd);
			fprintf(stderr, "Timeout when receiving response.\n");
			return 1;
		} else if (recv_bytes == GETSOCKOPT_ERROR) {
			close(socketfd);
			fprintf(stderr, "Could not get the socket option.\n");
			return 1;
		}

		if (buf[0] != JOKER_RESPONSE_TYPE) {
			close(socketfd);
			fprintf(stderr, "Response is of a different type.\n");
			return 1;
		}

		struct response_header * ans = (response_header *) buf;
		char *message = buf + sizeof(response_header);
		recv_bytes_sum += recv_bytes;
		len_of_joke = ntohl(ans->joke_length);
		char joke_part[len_of_joke];
		strncpy(joke_part, message, len_of_joke);
		joke_part[len_of_joke + 1] = '\0';
		printf("pure joke: %s\n", joke_part);
		//	} while (len_of_joke > recv_bytes_sum);
		printf("\n");
		close(socketfd);
	}
	return 0;
}
