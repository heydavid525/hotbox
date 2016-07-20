#pragma once
#include "data_util.hpp"
#include <vector>
#include "sparse_vec.hpp"
#include <cstdint>
#include <memory>
#include <glog/logging.h>

#include <hotbox/client/hb_client.hpp>
//#include <hotbox/schema/flexi_datum.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <memory.h>
#include <vector>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sstream>

#define TYPE_CREATE_SESSION 0x01
#define TYPE_GET_DATA 0x02

#define HEADER_LEN 5
#define BODY_LEN 1024
#define DATA_LEN (1024*1024*100)
#define NUM_SLICE 100000

#define BACKLOG 5
#define SERVER_PORT 13579


namespace petuum {
namespace fm {

struct DataConfig {
  int work_partition_id;
  int num_work_partitions;

  // Hotbox parameters.
  bool use_hotbox{false};
  hotbox::Session* hb_session{nullptr};
  int hb_num_transform_threads{4};

  // file reading parameters.
  std::string file_path;
  bool feature_one_based;
  // start reading from datum 'start_id'.
  int64_t start_id{0};
  int64_t num_data{0};
  std::string db_name{""};
  std::string session_name{""};
  std::string trans_conf{""};
  std::string dense{"false"};
};

class Data {
public:
  // Data does not own hb_session
  Data(const DataConfig& config) :
  use_hotbox_(config.use_hotbox), hb_session_(config.hb_session),
  config_(config) {
    if (config.num_data == 0) {
      LOG(INFO) << "No data. Disable the data loading.";
      return;
    }
    if (use_hotbox_) {
        CHECK_NOTNULL(hb_session_);
//      InitSocket();
//      CreateSession();

      num_data_this_partition_ = config.num_data / config.num_work_partitions;
      int64_t data_begin = config.start_id + config.work_partition_id
        * num_data_this_partition_;
      int64_t data_end = data_begin + num_data_this_partition_;
      data_end = 1000;
      LOG(INFO) << "Aout to GetTransformedData" ;
  //    GetTransformedData(data_begin, data_end);



      iter_.reset(hb_session_->NewDataIteratorPtr(data_begin, data_end,
            config.hb_num_transform_threads));
    } else {
      // TODO(wdai): LibSVM doesn't support start_id and reads the entire
      // file.
      ReadLibSVMPartition(config.file_path,
          config.work_partition_id, config.num_work_partitions,
          &X_train_, &y_train_, config.feature_one_based);
      num_data_this_partition_ = X_train_.size();
    }
  }

  // Return number of data in this partition.
  int64_t GetNumData() const {
    return num_data_this_partition_;
  }

  void GetTransformedData(int64_t data_begin, int64_t data_end) { 
    
    LOG(INFO) << "GetTransformedData: " << data_begin << " " << data_end;
    
    char header[HEADER_LEN];
    std::ostringstream oss;
    oss << data_begin << " " << data_end;
    std::string body = oss.str();
    LOG(INFO) << "data begin and end: " << body;
    CreateHeader(TYPE_GET_DATA, header, body.size());
    LOG(INFO) << "created header";
  
    send(server_socket_, header, HEADER_LEN, 0);
    LOG(INFO) << "sent header";
/*    send(server_socket_, body.c_str(), body.size(), 0);

    int slice_len = (data_end - data_begin) / NUM_SLICE;
    
    for(int slice = 1; slice <= NUM_SLICE; slice++) {
	char length_buf[4];
	int length;
	recv(server_socket_, length_buf, 4, 0);
	length = ((length_buf[1]<<24)|(length_buf[2]<<16)|
		(length_buf[3]<<8)|length_buf[4]);
	LOG(INFO) << "Read data length: " << length;
	char buf[DATA_LEN];
	recv(server_socket_,buf,length,0);
	LOG(INFO) << "Read the real data: " << length;
	int idx = 0;
	for(int slice_idx=0; slice_idx < slice_len; slice_idx++) {
            int feature_len = ReadInteger32(buf, idx); 
	    int64_t feature_dimension = ReadInt64(buf, idx);
	    float label = ReadFloat(buf, idx);
	    for(int feature_len_idx=12; feature_len_idx < feature_len; ){
	    	int64_t feature_idx = ReadInt64(buf, idx);
	    	float feature_val = ReadFloat(buf, idx);
		feature_len_idx += 12;
	    }
	}	
    } 
 */ 
    
  }

  void CreateSession() {
    std::string body = config_.db_name + " " +
			config_.session_name + " " + 
			config_.trans_conf + " " + 
			config_.dense;
    char header[HEADER_LEN];
    CreateHeader(TYPE_CREATE_SESSION, header, body.size());
    send(server_socket_, header, HEADER_LEN, 0);   
    send(server_socket_, body.c_str(), body.size(), 0); 
    char ack[1];
    recv(server_socket_, ack, 1, 0);
    LOG(INFO) << "Successfully created session: ";
    LOG(INFO) << "Session Info: " << std::endl
		<< "db_name: " << config_.db_name << std::endl
		<< "session_name: " << config_.session_name << std::endl
		<< "trans_conf: " << config_.trans_conf << std::endl;
  }

  void CreateHeader(int type, char* header, int length){
    header[0] = type;
    header[1] = (char)(length >> 24);
    header[2] = (char)(length >> 16);
    header[3] = (char)(length >> 8);
    header[4] = (char)(length);
  }

  int64_t ReadInt64(char* d, int& idx){
   double a;
   unsigned char *dst = (unsigned char *)&a;
   unsigned char *src = (unsigned char *)d;
   idx += 8;
   dst[0] = src[7];
   dst[1] = src[6];
   dst[2] = src[5];
   dst[3] = src[4];
   dst[4] = src[3];
   dst[5] = src[2];
   dst[6] = src[1];
   dst[7] = src[0];

   return a;
  }

 // int64_t ReadInteger64(char* data, int& idx) {
 //   idx += 8;
 //   int64_t ret;
 //   ret = ( (data[0]<<56) | (data[1]<<48) | (data[2]<<40) | (data[3]<<32) |
//		(data[4]<<24) | (data[5]<<16) | (data[6]<<8) | data[7]);
 //   return ret;
 //}
  
  float ReadFloat(char* inFloat, int& idx ){
   idx += 4;
   float retVal;
   char *floatToConvert = inFloat;
   char *returnFloat = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
  }
  
  int ReadInteger32(char* data, int& idx) {
    idx += 4;
    int ret;
    char* header = data;
    ret = ((header[1]<<24)|(header[2]<<16)|(header[3]<<8)|header[4]); 
    return ret;
  }

  std::pair<const SparseVec*, int> GetNext() {
    CHECK(HasNext());
    if (use_hotbox_) {
      hotbox::FlexiDatum datum = iter_->GetDatum();
      cache_vec_ = SparseVec(datum.MoveSparseIdx(),
          datum.MoveSparseVals());
      return std::make_pair(&cache_vec_,
          static_cast<int>(datum.GetLabel()));
    }
    return std::make_pair(&X_train_[curr_], y_train_[curr_++]);
  }

  void InitSocket() {
    sockaddr_in serverAddr;
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0){
	LOG(INFO) << "server socket error.";
	exit(1);
    }
    LOG(INFO) << "Server socket OK.";
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
//    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, "172.31.0.232", &serverAddr.sin_addr)<=0){
	LOG(INFO) << "inet_pton error occurred.";
    }
    if( connect(server_socket_, (struct sockaddr *)&serverAddr, 
	sizeof(serverAddr)) < 0) {
	LOG(INFO) << "Connect Failed.";	
    }  
  }
  void Restart() {
    if (use_hotbox_) {
      iter_->Restart();
    }
    curr_ = 0;
  }

  bool HasNext() const {
    if (use_hotbox_) {
      return iter_->HasNext();
    }
    return curr_ < X_train_.size();
  }

private:
  bool use_hotbox_;
  hotbox::Session* hb_session_{nullptr};
  std::unique_ptr<hotbox::DataIterator> iter_;
  int64_t num_data_this_partition_{0};
  // Need to cache the last returned SparseVec for the returned reference to
  // be valid.
  SparseVec cache_vec_;

  std::vector<SparseVec> X_train_;
  std::vector<int32_t> y_train_;
  int64_t curr_{0};

  DataConfig config_;
  int server_socket_;
};

} // namespace fm
} // namespace petuum
