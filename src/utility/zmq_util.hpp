#pragma once

#include <zmq.hpp>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <string>

namespace petuum {

class ZMQUtil {
public:
  static zmq::context_t* CreateZmqContext(int num_zmq_threads = 1);

  static std::string Convert2ZmqId(const std::string& id_str);

  static void ZMQSetSockOpt(zmq::socket_t *sock, int option,
      const void *optval, size_t optval_size);

  static void ZMQBind(zmq::socket_t *sock, const std::string &connect_addr);

  static void ZMQConnect(zmq::socket_t *sock,
      const std::string& connect_addr);

  static void ZMQConnectSend(zmq::socket_t *sock,
      const std::string& connect_addr, 
      int32_t zmq_id, void *msg, size_t size);

  // True for received, false for not
  static bool ZMQRecvAsync(zmq::socket_t *sock, zmq::message_t *msg);

  static bool ZMQRecvAsync(zmq::socket_t *sock, int32_t *zmq_id, zmq::message_t *msg);

  static void ZMQRecv(zmq::socket_t *sock, zmq::message_t *msg);
  
  static void ZMQRecv(zmq::socket_t *sock, int32_t *zmq_id, zmq::message_t *msg);

  /*
   * return number of bytes sent
   */
  static size_t ZMQSend(zmq::socket_t *sock, const void *data, size_t len, 
      int flag = 0);

  // 0 means cannot be sent, try again; 
  // should not happen unless flag = ZMQ_DONTWAIT
  static size_t ZMQSend(zmq::socket_t *sock, int32_t zmq_id, const void *data, 
    size_t len, int flag = 0);

  // msg is nollified during the call
  static size_t ZMQSend(zmq::socket_t *sock, zmq::message_t &msg, int flag = 0);

  static size_t ZMQSend(zmq::socket_t *sock, int32_t zmq_id, 
    zmq::message_t &msg, int flag = 0);


};
}
