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

int processServerResponse(int socketfd) {
	int recv_bytes;
	uint32_t len_of_joke = 0;
	char response_buffer[1000];

	//get the length of a joke from the server
	recv_bytes = recvtimeout(socketfd, response_buffer, sizeof(response_buffer));

	if (recv_bytes == ERROR_RESULT) {
		close(socketfd);
		fprintf(stderr, "Error during data receive.\n");
		return -1;
	} else if (recv_bytes == TIMEOUT_RESULT) {
		close(socketfd);
		fprintf(stderr, "Timeout when receiving response.\n");
		return -1;
	} else if (recv_bytes == GETSOCKOPT_ERROR) {
		close(socketfd);
		fprintf(stderr, "Could not get the socket option.\n");
		return -1;
	}

	struct response_header * ans = (response_header *) response_buffer;
	if (ans->type != JOKER_RESPONSE_TYPE) {
		close(socketfd);
		fprintf(stderr, "Response is of a different type.\n");
		return -1;
	}
	len_of_joke = ntohl(ans->joke_length);
	char *message = response_buffer + sizeof(response_header);

	//if server sends more than just a joke
	if (len_of_joke < strlen(message)) {
		char joke_part[len_of_joke];
		strncpy(joke_part, message, len_of_joke);
		joke_part[len_of_joke] = '\0';
		printf("joke: %s\n", joke_part);
	} else {
		printf("joke: %s\n", message);
	}
	close(socketfd);

	return 0;
}

char* removeNewLine(char *s) {
	int len = strlen(s);
	if (len > 0 && s[len - 1] == '\n') {
		s[len - 1] = '\0';
	}
	return s;
}

int main(int argc, char *argv[]) {
	//connecion data
	int socketfd;
	int bytes_sent;

	//exchange data
	char first_name[NAME_MAX_LENGTH];
	char last_name[NAME_MAX_LENGTH];
	int request_struct_size;
	request_header *request_msg;

	if (argc != REQUIRED_NUMBER_OF_CMD_ARGUMENTS) {
		fprintf(stderr, "Hostname and port were not provided by the user.\n");
		return 1;
	}

	//request the first name and last name
	printf("Please, enter your first name: ");
	char *first_name_pointer = fgets(first_name, NAME_MAX_LENGTH, stdin);
	first_name_pointer = removeNewLine(first_name_pointer);

	printf("Please, enter your last name: ");
	char *last_name_pointer = fgets(last_name, NAME_MAX_LENGTH, stdin);
	last_name_pointer = removeNewLine(last_name_pointer);

	uint8_t first_name_length = strlen(first_name_pointer);
	uint8_t second_name_length = strlen(last_name_pointer);
	request_struct_size = sizeof(struct request_header);
	int buffer_size = request_struct_size + first_name_length + second_name_length;
	char buffer[buffer_size];
	request_msg = (request_header *) buffer;

	request_msg->type = JOKER_REQUEST_TYPE;
	request_msg->len_first_name = first_name_length;
	request_msg->len_last_name = second_name_length;

	char * payload = buffer + request_struct_size;

	strncpy(payload, first_name_pointer, first_name_length + 1);
	strcat(payload, last_name_pointer);

	socketfd = setupsocket(argv);
	if (socketfd != -1) {
		bytes_sent = send(socketfd, buffer, buffer_size, 0);
		if (bytes_sent == -1) {
			close(socketfd);
			fprintf(stderr, "No bytes have been sent.\n");
			return 1;
		}
		if (processServerResponse(socketfd) != 0) {
			return 1;	//error in processing response
		}
	}
	return 0;
}
