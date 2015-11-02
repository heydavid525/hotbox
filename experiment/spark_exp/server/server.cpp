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
using namespace hotbox;


Session *session;

void memcpyInt(unsigned char *dst, const unsigned char *src)
{
	memcpy(dst++, src + 3, 1);
	memcpy(dst++, src + 2, 1);
	memcpy(dst++, src + 1, 1);
	memcpy(dst++, src + 0, 1);
};

void handleCreate(int clientSocket, int length){
	char body[BODY_LEN];
	if(recv(clientSocket, body, length, 0) <= 0)
		return;
		
	char db_name[BODY_LEN], session_id[BODY_LEN], transform_config_path[BODY_LEN], is_dense[10];
	sscanf(body, "%s %s %s %s", db_name, session_id, transform_config_path, is_dense);
	printf("session option : %s %s %s %s\n", db_name, session_id, transform_config_path, is_dense);
	SessionOptions session_options;

	session_options.db_name = db_name;
	session_options.session_id = session_id;
	session_options.transform_config_path = transform_config_path;
	if(strcmp(is_dense, "true"))
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
		
	int64_t begin, end;
	sscanf(body, "%ld %ld", &begin, &end);
	printf("get data : %ld %ld", begin, end);
	stringstream data;
	for (DataIterator it = session->NewDataIterator(begin, end); it.HasNext(); it.Next()) {
		data << it.GetDatum().GetFeatureDim() << " "
		  << it.GetDatum().ToLibsvmString() << ";;;";		
		printf("tmp_len: %ld\n", data.str().length());
	}
	unsigned char response[4];
	int data_len = data.str().length() + 1;
	printf("data_len: %d\n", data_len);
	memcpyInt(response, (unsigned char*)&data_len);
	send(clientSocket, response, 4, 0);
	send(clientSocket, data.str().c_str(), data_len, 0);	
}

void doClientRequest(int clientSocket){
    while(1){
		int length;
		char header[HEADER_LEN];
		if(recv(clientSocket, header, HEADER_LEN, 0) <= 0)
			return;
		printf("%x %x %x %x %x\n", header[0], header[1], header[2], header[3], header[4]);
		length=((header[1]<<24)|(header[2]<<16)|(header[3]<<8)|header[4]);
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