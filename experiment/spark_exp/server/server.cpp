#include "client/hb_client.hpp"
#include "schema/flexi_datum.hpp"

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
#include <boost/thread/thread.hpp>

#define TYPE_CREATE_SESSION 0x01
#define TYPE_GET_DATA 0x02

#define HEADER_LEN 5
#define BODY_LEN 1024
#define DATA_LEN (1024*1024*100)
#define NUM_SLICE 10000

#define BACKLOG 5
#define LISTENT_PORT 13579

using namespace std;
using namespace hotbox;

Session* session = NULL;

inline void memcpy4(char *dst, const char *src)
{
	*dst = *(src + 3);
	*(dst + 1) = *(src + 2);
	*(dst + 2) = *(src + 1);
	*(dst + 3) = *(src + 0);
};

inline void memcpy8(char *dst, const char *src)
{
	*dst = *(src + 7);
	*(dst + 1) = *(src + 6);
	*(dst + 2) = *(src + 5);
	*(dst + 3) = *(src + 4);
	*(dst + 4) = *(src + 3);
	*(dst + 5) = *(src + 2);
	*(dst + 6) = *(src + 1);
	*(dst + 7) = *(src + 0);
};

void handleCreate(int clientSocket, int length){
	char body[BODY_LEN];
	if(recv(clientSocket, body, length, 0) <= 0)
		return;
		
	char db_name[BODY_LEN], session_id[BODY_LEN], transform_config_path[BODY_LEN], is_dense[10];
	sscanf(body, "%s %s %s %s", db_name, session_id, transform_config_path, is_dense);
	printf("session option : %s %s %s %s\n", db_name, session_id, transform_config_path, is_dense);
	SessionOptions session_options;

	session_options.db_name = string(db_name);
	session_options.session_id = string(session_id);
	session_options.transform_config_path = string(transform_config_path);
	if(strcmp(is_dense, "true") == 1)
		session_options.output_store_type = OutputStoreType::DENSE;
	else
		session_options.output_store_type = OutputStoreType::SPARSE;
	printf("before hb client\n");
	HBClient hb_client;
	printf("before session\n");
	Session* session = hb_client.CreateSessionWithPointer(session_options);
	printf("after session\n");
	char response[1];
	response[0] = 0x00;
	send(clientSocket, response, 1, 0);
	return;
}

void handleGet(int clientSocket, int length){
	char body[BODY_LEN];
	
	if(recv(clientSocket, body, length, 0) <= 0)
		return;
	
	body[length] = '\0';
	int64_t begin, end;
	sscanf(body, "%ld %ld", &begin, &end);
	printf("get data : %ld %ld", begin, end);
	
	int64_t num_data;
	if(end == -1)
		num_data = session->GetNumData() - begin;
	else
		num_data = end - begin;
	printf("num data %ld", num_data);
	
	int slice_len = num_data / NUM_SLICE;
	DataIterator it = session->NewDataIterator(begin, end); 

	for(int slice = 1; slice <= NUM_SLICE; slice++){
		char* data = new char[DATA_LEN];
		char* p = data;
		for (int tmp_len = 0; it.HasNext() && ((slice == NUM_SLICE) || ((slice != NUM_SLICE) && (tmp_len < slice_len))); it.Next()) {
			char datumData[1024 * 100];
			char *dst = datumData;
			auto datum = it.GetDatum();	
			long featureDim = datum.GetFeatureDim();
			memcpy8(dst, (char*)&featureDim);
			dst += 8;
			float label = datum.GetLabel();
			memcpy4(dst, (char*)&label);
			dst += 4;
			auto idx = datum.GetSparseIdx();
			auto val = datum.GetSparseVals();
			for(int i = 0; i < idx.size(); i++){
				int featureIdx = idx[i];
				memcpy4(dst, (char*)&featureIdx);
				dst += 4;
				memcpy4(dst, (char*)&(val[i]));
				dst += 4;
			}
			int byteCount = dst - datumData;
			memcpy4(p, (char*)&byteCount);
			p += 4;
			memcpy(p, datumData, byteCount);
			p += byteCount;
			tmp_len++;
		}
		char response[4];
		int data_len = p - data;
		//printf("data_slice_len: %d\n", data_len);
		memcpy4(response, (char*)&data_len);
		send(clientSocket, response, 4, 0);
		send(clientSocket, data, data_len, 0);	
		delete data;
	}
}

void doClientRequest(int clientSocket){
    while(1){
		int length;
		char header[HEADER_LEN];
		if(recv(clientSocket, header, HEADER_LEN, 0) <= 0)
			break;
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

void threadRunable(int clientSocket){
	doClientRequest(clientSocket);
	close(clientSocket);
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
		boost::thread thd(threadRunable, clientSocket);		
    }	
}

void initHB(){
	SessionOptions session_options;

	session_options.db_name = "url_combined";
	session_options.session_id = "url_session";
	session_options.transform_config_path = "/home/wanghy/github/hotbox/test/resource/select_all.conf";
	session_options.output_store_type = OutputStoreType::SPARSE;
	
	HBClient hb_client;
	session = hb_client.CreateSessionWithPointer(session_options);
}

int main(int argc, char** argv){
	initHB();
    run_server();
	if(session != NULL)
		delete session;
	session = NULL;
    return 0;
}