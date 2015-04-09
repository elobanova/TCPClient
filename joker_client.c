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
#define MAX_BUF_SIZE_FOR_JOKE 1024
#define PROCESS_SERVER_RESPONSE_ERROR -1

int printErrorAndCloseSocket(int recv_bytes, int socketfd) {
	if (recv_bytes == ERROR_RESULT) {
		close(socketfd);
		fprintf(stderr, "Error during data receive.\n");
		return PROCESS_SERVER_RESPONSE_ERROR;
	} else if (recv_bytes == TIMEOUT_RESULT) {
		close(socketfd);
		fprintf(stderr, "Timeout when receiving response.\n");
		return PROCESS_SERVER_RESPONSE_ERROR;
	} else if (recv_bytes == GETSOCKOPT_ERROR) {
		close(socketfd);
		fprintf(stderr, "Could not get the socket option.\n");
		return PROCESS_SERVER_RESPONSE_ERROR;
	} else if (recv_bytes == CONNECTION_CLOSED_ERROR) {
		close(socketfd);
		fprintf(stderr, "Connection was closed.\n");
		return PROCESS_SERVER_RESPONSE_ERROR;
	}

	return 0;
}

int processServerResponse(int socketfd) {
	//header response data
	int recv_bytes_for_header;
	uint32_t len_of_joke = 0;
	int response_header_struct_size = sizeof(response_header);
	char *total_response_header_buffer = (char *) malloc(response_header_struct_size);
	struct response_header * ans;

	//joke response data
	uint32_t total_recv_bytes_for_joke = 0;
	int left_bytes_to_read = 0;
	int recv_bytes_for_joke;
	char buffer[MAX_BUF_SIZE_FOR_JOKE];
	char* chunk_response_joke_buffer = buffer;
	char response_joke_buf[MAX_BUF_SIZE_FOR_JOKE];
	response_joke_buf[0] = '\0';

	//get the length of a joke from the server
	recv_bytes_for_header = recvtimeout(socketfd, total_response_header_buffer, response_header_struct_size);
	if (printErrorAndCloseSocket(recv_bytes_for_header, socketfd) == -1) {
		return PROCESS_SERVER_RESPONSE_ERROR;
	}

	ans = (response_header *) total_response_header_buffer;
	if (recv_bytes_for_header < response_header_struct_size || ans->type != JOKER_RESPONSE_TYPE) {
		close(socketfd);
		fprintf(stderr, "A server did not respond with the proper header.\n");
		return PROCESS_SERVER_RESPONSE_ERROR;
	}
	len_of_joke = ntohl(ans->joke_length);

	//get the joke from the server
	do {
		left_bytes_to_read = len_of_joke - total_recv_bytes_for_joke;
		recv_bytes_for_joke = recvtimeout(socketfd, chunk_response_joke_buffer, left_bytes_to_read);
		if (printErrorAndCloseSocket(recv_bytes_for_joke, socketfd) == PROCESS_SERVER_RESPONSE_ERROR) {
			return PROCESS_SERVER_RESPONSE_ERROR;
		}
		total_recv_bytes_for_joke += recv_bytes_for_joke;
		chunk_response_joke_buffer[recv_bytes_for_joke] = '\0';
		strcat(response_joke_buf, chunk_response_joke_buffer);
		chunk_response_joke_buffer += recv_bytes_for_joke;
	} while (total_recv_bytes_for_joke < len_of_joke && total_recv_bytes_for_joke < MAX_BUF_SIZE_FOR_JOKE);

	printf("The whole joke: %s\n", response_joke_buf);
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
