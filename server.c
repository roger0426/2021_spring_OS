#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"

#define LEN_NAME 20
#define LEN_MSG 50
#define LEN_SEND 100

// Global variables
ClientList *root, *current;
pthread_mutex_t mutex;
int ser_skt = 0, cli_skt = 0;

//used by signal function
//when signal interrupt occur use this function to close all socket
void ctrl_c_exit(){
    ClientList *tmp;
    while (root != NULL) {
        printf("\nClose socket: %d\n", root->data);
        close(root->data); // close all socket include ser_skt
        tmp = root;
        root = root->next;//point to next Clientnode
        free(tmp);
    }
    printf("Bye~~\n");
    exit(EXIT_SUCCESS);
}

//use this function to send the message to all the client
void send_all(ClientList *cli, char tmp_buffer[]){
    ClientList *tmp = root->next;
    while (tmp != NULL) {
        if (cli->data != tmp->data) { // all clients except itself.
            printf("Send to socket #%d: \"%s\" \n", tmp->data, tmp_buffer);
            //tmp_buffer is the message
            //LEN_SEND is the length of message
            //tmp->data is the socket created before
            send(tmp->data, tmp_buffer, LEN_SEND, 0);
        }
        tmp = tmp->next;
    }
}
//to deal with the connection
void handler(void *pth_cli) {
    int leave = 0;
    char username[LEN_NAME];
    char message_buffer[LEN_MSG];
    char store_send[LEN_SEND];
    //create a client node
    ClientList *cli = (ClientList *)pth_cli;

    //recv() is used to receive message from socket
    //if username length exceed the range it will ask the client to enter again
    //if the format is correct it will print the information of client's neckname
    if(recv(cli->data, username, LEN_NAME, 0) <= 0 || strlen(username) == 0 || strlen(username) >= LEN_NAME-1){
        printf("ERROR FORMAT!\n");
        leave = 1;
    } 
    else{
        strncpy(cli->name, username, LEN_NAME);
        printf("%s(%s)(%d) enter the chatroom.\n", cli->name, cli->ip, cli->data);
        sprintf(store_send, "%s(%s) enter the chatroom.", cli->name, cli->ip);
        send_all(cli, store_send);
    }

    for(;;) {
        if (leave) break;
        int receive = recv(cli->data, message_buffer, LEN_MSG, 0);
        //receive will return the length of message on successful completion
        //if message_buffer length is 0 means there didn't contain any message need to be sended
        //if it contain the message in message_buffer then it will cope it to store_send
        //if client enter exit then the his connection will be suspend
        //if receive smaller than 0 means it didn't complete successfully
        pthread_mutex_lock(&mutex);
        if (receive > 0){
            if (strlen(message_buffer) == 0){
                continue;
            }
            sprintf(store_send, "%s：%s from %s", cli->name, message_buffer, cli->ip);
        } 
        else if(receive == 0 || strcmp(message_buffer, "exit") == 0){
            printf("%s(%s)(%d) leave the chatroom.\n", cli->name, cli->ip, cli->data);
            sprintf(store_send, "%s(%s) leave the chatroom.", cli->name, cli->ip);
            leave = 1;
        } 
        else{
            printf("Error: -1\n");
            leave = 1;
        }
        //use the send_all to send the message to all client
        send_all(cli, store_send);
        pthread_mutex_unlock(&mutex);
    }

    // Remove Node
    pthread_mutex_lock(&mutex);
    close(cli->data);
    if(cli == current){ // remove an edge node
        current = cli->prev;
        current->next = NULL;
    } 
    else{ // remove a middle node
        //break the remove node's next from previous node and next node
        cli->prev->next = cli->next;
        cli->next->prev = cli->prev;
    }
    free(cli);
    pthread_mutex_unlock(&mutex);
}

int main()
{
    //SIGINT(Signal Interrupt) Interactive attention signal. Generally generated by the application user
    //catch_ctrl_c_and_exist is self define function
    signal(SIGINT, ctrl_c_exit);
    // Create socket use the domain with IPv4 internet protocol version 4
    //type SOCK_STREAM provide double-sided  Transmission Control Protocol
    //protocol 0 will use the preset mode which is dependent on domain and type
    //return a file descriptor for new socket if success,return -1 if fail
    ser_skt = socket(AF_INET , SOCK_STREAM , 0);
    //fail to create socket (exit)
    if(ser_skt == -1){
        printf("Fail to create a socket.\n");
        exit(EXIT_FAILURE);
    }
    // Socket information
    struct sockaddr_in ser_info, cli_info;
    int ser_len = sizeof(ser_info);
    int cli_len = sizeof(cli_info);
    //clean memory fill it with 0
    memset(&ser_info, 0, ser_len);
    memset(&cli_info, 0, cli_len);
    ser_info.sin_family = PF_INET;//Given a socket name
    ser_info.sin_addr.s_addr = INADDR_ANY;//Given the host address
    //INADDR_ANY: 0.0.0.0, accept all IPs of the machine
    ser_info.sin_port = htons(8000);//Given a port number

    // Bind and Listen
    //sever_socket is the socket created above
    //use bind to next the IP address with sever
    //use listen to wait for the request from client
    bind(ser_skt, (struct sockaddr *)&ser_info, ser_len);
    listen(ser_skt, 5);

    // Print Server IP
    // use getsockname to get the information of IP and port
    getsockname(ser_skt, (struct sockaddr*) &ser_info, (socklen_t*) &ser_len);
    printf("Start Server on: %s:%d\n", inet_ntoa(ser_info.sin_addr), ntohs(ser_info.sin_port));

    // Initial nexted list for clients
    //inet_nota is to convert the internet binary number (sever_info.sin_addr) into address
    root = newNode(ser_skt, inet_ntoa(ser_info.sin_addr));
    current = root;
	pthread_mutex_init(&mutex, NULL);
    for(;;){
        //after bind and listen when there have the connection it will handle it and return a new socket to cope with next connection
        //if connection success the structure &client_info point to will add into host information
        cli_skt = accept(ser_skt, (struct sockaddr*) &cli_info, (socklen_t*) &cli_len);

        // Print Client IP
        //getpeername return the address and port of the client
        //inet_ntoa transfer address to string with "."
        //ntohs is used to convert number between network byte order and the order of powerpc
        getpeername(cli_skt, (struct sockaddr*) &cli_info, (socklen_t*) &cli_len);
        printf("Client %s:%d join.\n", inet_ntoa(cli_info.sin_addr), ntohs(cli_info.sin_port));

        // Append nexted list for clients
        ClientList *c = newNode(cli_skt, inet_ntoa(cli_info.sin_addr));
        //point the new node's prev to current then now's next point to new node
        //make the new client node become current node
        c->prev = current;
        current->next = c;
        current = c;

        //create pthread to handle the connection
        pthread_t pid;
        //client_handler is the self defin function
        //if pthread create successfully then it will handle the sending task
        pthread_create(&pid, NULL, (void *)handler, (void *)c);
    }

    return 0;
}