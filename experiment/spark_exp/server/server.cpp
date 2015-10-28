#include "spark_exp.pb.h"
#include "client/hb_client.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <iostream>
#include <memory.h>
#include <vector>
#include <signal.h>
#include <time.h>

#define TYPE_CREATE_SESSION 0x01
#define TYPE_GET_DATA 0x02

#define HEADER_LEN 5
#define BODY_LEN 1024

#define BACKLOG 5
#define LISTENT_PORT 13579

using namespace std;
using namespace spark_exp;
using namespace hotbox;


Session *session;

void handleCreate(int clientSocket, int length){
	char body[BODY_LEN];
	if(recv(clientSocket, body, length, 0) <= 0)
		return;
	SessionOptions session_options;
	Session_option_msg som;
	if (!som.ParseFromString(string(body))) {
      printf("Failed to parse Session_option_msg \n");
      return;
    }
	session_options.db_name = som.db_name();
	session_options.session_id = som.session_id();
	session_options.transform_config_path = som.transform_config_path();
	if(som.is_dense())
		session_options.output_store_type = OutputStoreType::DENSE;
	else
		session_options.output_store_type = OutputStoreType::SPARSE;
	
	HBClient hb_client;
	session = hb_client.CreateSessionWithPointer(session_options);
	char response[1];
	response[0] = 0x00;
	send(clientSocket, response, 1, 0);
}

void handleGet(int clientSocket, int length){
	char body[BODY_LEN];
	
	if(recv(clientSocket, body, length, 0) <= 0)
		return;
	Data_request_msg drm;
	if (!drm.ParseFromString(string(body))) {
      printf("Failed to parse Data_request_msg \n");
      return;
    }
	Libsvm_data_msg ldm;
	
	for (DataIterator it = session->NewDataIterator(drm.begin(), drm.end()); it.HasNext(); it.Next()) {
		string datum = it.GetDatum().ToString();
		ldm.add_data(datum);		
	}
	string ser;	
	ldm.SerializeToString(&ser);
	char response[4];
	int serLen = ser.length();
	memcpy(response, &serLen, 4);
	send(clientSocket, response, 4, 0);
	send(clientSocket, ser.c_str(), serLen, 0);	
}

void doClientRequest(int clientSocket){
    while(1){
		int length;
		char header[HEADER_LEN];
		if(recv(clientSocket, header, HEADER_LEN, 0) <= 0)
			return;
		memcpy(&length, &header[1], 4);
		printf("length: %d\n", length);
		
		if(header[0] == TYPE_CREATE_SESSION){
			printf("recieve create session \n");
			handleCreate(clientSocket, length);
		}
		else if(header[0] == TYPE_GET_DATA){
			printf("recieve get data \n");
			handleGet(clientSocket, length);
		}
		else{
			printf("unknown req type\n");
		}
	}
}

void run_server(){
	int serverSocket;
    int clientSocket;
    struct sockaddr_in serverAddr;

    serverSocket = socket(AF_INET,SOCK_STREAM, 0);
    if (serverSocket < 0){
        printf("server socket error\n");
        exit(1);
    }
    printf("server socket ok\n");
    bzero(&serverAddr,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(LISTENT_PORT);
    if (bind(serverSocket,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 ){
        printf("bind error\n");
        exit(1);
    }
    printf("bind ok\n");
    if (listen(serverSocket, BACKLOG) < 0){
        printf("listen error\n");
        exit(1);
    }
    printf("listening clients...\n");
    while(1){
        printf("waiting accept\n");
        clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0){
            printf("accept  error");
            exit(1);
        }
        printf("client accepted,socket:%d\n", clientSocket);
		doClientRequest(clientSocket);
		close(clientSocket);
    }	
}

int main(int argc, char** argv){
	//printf("111\n");
    run_server();
    return 0;
}