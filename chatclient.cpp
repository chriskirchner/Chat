/*
 * =====================================================================================
 *
 *       Filename:  chatclient.cpp
 *
 *    Description:  chat client that communicates with chat server using TCP connection
 *
 *        Version:  1.0
 *        Created:  10/30/2016 07:01:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Kirchner (CLK), kirchnch@oregonstate.edu
 *   Organization:  OSU
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

//define max application msg size to send through socket
#define MAXDATASIZE 500
//define max client handle size
#define MAXHANDLESIZE 10

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getAddrInfo 
 *  Description:  wrapper function for getaddrinfo that returns host address information
 *  and resolves hostname to IP address
 * =====================================================================================
 */

struct addrinfo *getAddrInfo(char hostname[], char port[]){

    //setup hint to seed address info
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    //un-specify the internet family
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //use host IP
    hints.ai_flags = AI_PASSIVE;

	//get addresss info and handle errors
    int status;
    struct addrinfo *ai = (struct addrinfo*) malloc(sizeof(struct addrinfo));
    if ((status = getaddrinfo(hostname, port, &hints, &ai) != 0)){
        fprintf(stderr, "failed to resolve host address: %s\n", gai_strerror(status));
        exit(1);
    }
    struct sockaddr_in *sin = (sockaddr_in*) ai->ai_addr;
    //printf("%s", inet_ntoa(sin->sin_addr));
    //fflush(stdout);

    return ai;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getSocket
 *  Description:  returns connected socket using host address information
 * =====================================================================================
 */

int getSocket(struct addrinfo *serv_info){
    struct addrinfo *ai = serv_info;
    int sock = 0;
	//iterate through addrinfo linked list
    while (ai != NULL && sock == 0){
		//get socket from addrinfo
        if ((sock = socket(serv_info->ai_family,
                           serv_info->ai_socktype,
                           serv_info->ai_protocol)) == -1){
            perror("client: failed to get socket");
        }

		//try to connect to socket
        if (connect(sock,
                    serv_info->ai_addr,
                    serv_info->ai_addrlen) == -1){

            close(sock);
            sock = 0;
            perror("client: failed to connect socket");
        }
        ai = ai->ai_next;
    }

    return sock;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  sendNum
 *  Description:  sends uint32_t number through socket
 * =====================================================================================
 */

ssize_t sendNum(uint32_t num, int sock){
    ssize_t sn;
	//convert to network big-endian
    num = htonl(num);
    if ((sn = send(sock, &num, sizeof num, 0)) == -1){
        perror("Failed to send value");
    }
	//check if proper bytes count sent
    else if (sn != sizeof num){
        perror("Failed to send correct value");
    }
    return sn;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  sendMsg
 *  Description:  sends char message through socket
 * =====================================================================================
 */

int sendMsg(char *msg, int sock){
    int msgSize = strlen(msg);
    ssize_t sn;

	//communicate msg length to host
    if ((sn = sendNum(msgSize, sock)) == -1){
        return -1;
    }

	//send message through socket in chunks
    int sent = 0;
    while (sent < msgSize){
        sn = send(sock, msg+sent, msgSize-sent, 0);
        if (sn == -1){
            return -1;
        }
        sent += (int) sn;
    }

    return 1;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getNum
 *  Description:  returns received uint32_t value from socket
 * =====================================================================================
 */

uint32_t getNum(int sock){
    ssize_t rn;
    uint32_t msgSize;
    if ((rn = read(sock, &msgSize, sizeof msgSize)) == -1){
        perror("Failed to get size of msg");
        return -1;
    }
	//return host converted value
    return ntohl(msgSize);
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getMsg
 *  Description:  returns received char message from socket
 * =====================================================================================
 */

char *getMsg(int sock){
    ssize_t rn;
	//get message size from host
    uint32_t msgSize = getNum(sock);

	//receive message from socket in chunks
    int received = 0;
    char *msg = (char*) malloc(msgSize+1);
    //make sure message is clean with nulls
    memset(msg, 0, msgSize+1);
    while (received < msgSize){
        if ((rn = read(sock, msg+received, msgSize-received)) == -1){
            perror("Failed to receive msg");
            return NULL;
        }
        received += rn;
    }

    return msg;
}

/*  INITIATES CONTACT */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  getMsg
 *  Description:  returns received char message from socket
 * =====================================================================================
 */


int getConnection(char *hostname, char *port){
    //get server address information
    struct addrinfo *serv_info = getAddrInfo(hostname, port);
    //get connected socket using server address information
    int sock = getSocket(serv_info);
    //free server addr info
    freeaddrinfo(serv_info);
    return sock;
}

void chatLoop(int sock, char* client_handle, char* serv_handle) {
    char client_msg[MAXDATASIZE + 1];
    char *serv_msg;
    int end = 0;
    while (!end) {
        printf("%s> ", client_handle);
        memset(client_msg, 0, MAXDATASIZE + 2);
        fgets(client_msg, MAXDATASIZE + 1, stdin);
        int i = 0;
        int newline = 0;
        while (!newline && i < MAXDATASIZE) {
            if (client_msg[i] == '\n') {
                client_msg[i] = '\0';
                newline = 1;
            }
            i++;
            if (!newline && i >= MAXDATASIZE) {
                //http://c-faq.com/stdio/stdinflush2.html
                char c;
                while ((c = getchar()) != '\n' && c != EOF) {
                    //discard character in standard buffer
                    continue;
                }
            }
        }
        if (strcmp(client_msg, "\\quit") == 0) {
            sendMsg(client_msg, sock);
            end = 1;
            printf("%s left the chat\n", client_handle);
        } else {
            sendMsg(client_msg, sock);
            memset(client_msg, 0, MAXDATASIZE + 1);
            serv_msg = getMsg(sock);
            if (strcmp(serv_msg, "\\quit") == 0) {
                end = 1;
                printf("%s left the chat\n", serv_handle);
            } else {
                printf("%s> %s\n", serv_handle, serv_msg);
            }
            free(serv_msg);
        }
    }
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  main function for communicating with chat server
 * =====================================================================================
 */

int main(int argc, char *argv[]) {
    if (argc != 3){
        fprintf(stderr, "usage: hostname port\n");
        exit(1);
    }

    //get connected socket using server port and hostname
    char *hostname = argv[1];
    char *port = argv[2];
    int sock = getConnection(hostname, port);


    //get client handle
    char client_handle[MAXHANDLESIZE];
    memset(client_handle, 0, MAXHANDLESIZE+2);
    printf("Input Handle: ");
    scanf("%s", client_handle);
    //capture newline
    fgetc(stdin);

    //get server handle
    char *serv_handle;
    sendMsg(client_handle, sock);
    serv_handle = getMsg(sock);
    printf("Connection established with: %s\n", serv_handle);

    //loop through chat communication until user quits
    chatLoop(sock, client_handle, serv_handle);
    close(sock);

    return 0;
}
