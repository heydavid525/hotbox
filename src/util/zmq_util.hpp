#pragma once

#include <zmq.hpp>
#include <string>
#include <cstdint>

namespace mldb {
namespace zmq_util {

zmq::context_t* CreateZmqContext(int num_zmq_threads = 1);

// ZMQ ROUTER socket id has to be at least 1 byte and cannot start with binary
// 0. Here we assume id_str is at least 1 byte long.  See
// http://api.zeromq.org/4-0:zmq-setsockopt
std::string Convert2ZmqId(const std::string& id_str);

void ZMQSetSockOpt(zmq::socket_t* sock, int option,
    const void* optval, size_t optval_size);

void ZMQBind(zmq::socket_t* sock, const std::string &connect_addr);

void ZMQConnect(zmq::socket_t* sock, const std::string& connect_addr);

// Return success or not.
bool ZMQSend(zmq::socket_t* sock, const std::string& dst_id,
    const std::string& data);

// Blocking receive.
zmq::message_t ZMQRecv(zmq::socket_t* sock, std::string* src_id = nullptr);

}   // zmq_util
}   // namespace mldb
