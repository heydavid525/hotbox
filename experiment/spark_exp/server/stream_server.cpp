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

#define HEADER_LEN 5
#define BODY_LEN 1024

#define BACKLOG 5
#define LISTENT_PORT 24680

using namespace std;
using namespace hotbox;

Session *session = NULL;

void memcpyInt(unsigned char *dst, const unsigned char *src)
{
	memcpy(dst++, src + 3, 1);
	memcpy(dst++, src + 2, 1);
	memcpy(dst++, src + 1, 1);
	memcpy(dst++, src + 0, 1);
};

void initHB(){
	SessionOptions session_options;

	session_options.db_name = "url_combined";
	session_options.session_id = "url_session";
	session_options.transform_config_path = "/home/wanghy/github/hotbox/test/resource/select_transform.conf";
	session_options.output_store_type = OutputStoreType::SPARSE;
	
	HBClient hb_client;
	session = hb_client.CreateSessionWithPointer(session_options);
}

void doClientRequest(int clientSocket){
    DataIterator it = session->NewDataIterator(0, -1); 
	
	int slice_len = 1000;
	int num_slice = session->GetNumData() / slice_len;
	for(int slice = 1; slice <= num_slice; slice++){
		stringstream data;
		for (int tmp_len = 0; it.HasNext() && ((slice == num_slice) || ((slice != num_slice) && (tmp_len < slice_len))); it.Next()) {
			data << it.GetDatum().GetFeatureDim() << " "
			  << it.GetDatum().ToLibsvmString() << "\n";		
			tmp_len++;
		}
		//unsigned char response[4];
		int data_len = data.str().length();
		printf("data_slice_len: %d\n", data_len);
		//memcpyInt(response, (unsigned char*)&data_len);
		//send(clientSocket, response, 4, 0);
		send(clientSocket, data.str().c_str(), data_len, 0);	
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
    //while(1){
        printf("waiting accept\n");
        clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0){
            printf("accept  error");
            exit(1);
        }		
        printf("client accepted,socket:%d\n", clientSocket);
		doClientRequest(clientSocket);
		close(clientSocket);
    //}	
}

int main(int argc, char** argv){
	initHB();
    run_server();
	if(session != NULL)
		delete session;
	session = NULL;
    return 0;
}