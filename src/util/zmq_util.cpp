#include <glog/logging.h>
#include <cstdint>
#include "zmq_util.hpp"

namespace hotbox {
namespace zmq_util {

zmq::context_t* CreateZmqContext(int num_zmq_threads) {
  try {
    // 0 IO thread since inproc comm doesn't need any.
    return new zmq::context_t(num_zmq_threads);
  } catch (zmq::error_t &e) {
    LOG(FATAL) << "Faield to create zmq context " << e.what();
  } catch (...) {
    LOG(FATAL) << "Failed to create zmq context";
  }
  return nullptr; // should never reach here.
}

std::string Convert2ZmqId(const std::string& id_str) {
  auto copy_str = id_str;
  // Prepend a byte so the leftmost bit is 1.
  uint8_t byte_start_with_1 = 1 << 7;
  //LOG(INFO) << "byte_start_with_1: " << byte_start_with_1;
  copy_str.insert(0, reinterpret_cast<char*>(&byte_start_with_1), 1);
  return copy_str;
}

void ZMQSetSockOpt(zmq::socket_t* sock, int option,
    const void* optval, size_t optval_size) {
  try {
    sock->setsockopt(option, optval, optval_size);
  } catch (zmq::error_t &e) {
    switch(e.num()){
      case EINVAL:
      case ETERM:
      case ENOTSOCK:
      case EINTR:
        LOG(FATAL) << e.what();
        break;
      default:
        LOG(FATAL) << e.what();
    }
  }
}

void ZMQBind(zmq::socket_t* sock, const std::string &connect_addr) {
  try {
    sock->bind(connect_addr.c_str());
  } catch (zmq::error_t &e) {
    switch (e.num()) {
      case EINVAL:
      case EPROTONOSUPPORT:
      case ENOCOMPATPROTO:
      case EADDRINUSE:
      case EADDRNOTAVAIL:
      case ENODEV:
      case ETERM:
      case ENOTSOCK:
      case EMTHREAD:
        LOG(FATAL) << e.what() << " connect_addr = " << connect_addr;
        break;
      default:
        LOG(FATAL) << e.what();
    }
  }
}

void ZMQConnect(zmq::socket_t* sock, const std::string& connect_addr) {
  try {
    sock->connect(connect_addr.c_str());
  } catch (zmq::error_t &e) {
    LOG(FATAL) << e.what();
  }
}

namespace {

// Low-level send.
bool ZMQSendInternal(zmq::socket_t* sock, const void* data, size_t len,
    int flag = 0) {
  try {
    size_t nbytes = sock->send(data, len, flag);
    return len == nbytes;
  } catch (zmq::error_t &e) {
    switch (e.num()) {
      case EHOSTUNREACH:
        LOG(INFO) << "The client dropped? " << e.what();
        break;
      case ENOTSUP:
      case EFSM:
      case ETERM:
      case ENOTSOCK:
      case EFAULT:
      case EAGAIN:
        // EAGAIN would not be thrown which got eaten by c++ binding.
        // These errors mean there are bugs in the code, fail fast
        LOG(FATAL) << e.what() << "Possibly due to a lost client";
        break;
      case EINTR:
        // I do not yet handle interrupt, so fail fast
        LOG(FATAL) << e.what();
        break;
      default:
        LOG(FATAL) << e.what();
        return false;
    }
  }
  return false;
}

}  // anonymous namespace


// High-level send.
bool ZMQSend(zmq::socket_t* sock, const std::string& dst_id,
    const std::string& data) {
  bool zid_sent = ZMQSendInternal(sock, dst_id.c_str(), dst_id.size(),
      ZMQ_SNDMORE);
  // Send an empty frame to be consistent with REQ socket behavior.
  bool empty_sent = ZMQSendInternal(sock, dst_id.c_str(), 0, ZMQ_SNDMORE);
  return zid_sent && empty_sent &&
    ZMQSendInternal(sock, data.c_str(), data.size());
}


namespace {

// Low-leve receive.
zmq::message_t ZMQRecvInternal(zmq::socket_t* sock) {
  bool recved = false;
  zmq::message_t msg;
  try {
    recved = sock->recv(&msg);
  } catch (zmq::error_t &e) {
    switch(e.num()){
      case ENOTSUP:
      case EFSM:
      case ETERM:
      case ENOTSOCK:
      case EFAULT:
      case EAGAIN: // EAGAIN should not be thrown
        // These errors mean there are bugs in the code, fail fast
        LOG(FATAL) << e.what();
        break;
      case EINTR:
        // I do not yet handle interrupt, so fail fast
        LOG(FATAL) << e.what();
        break;
      default:
        LOG(FATAL) << e.what();
    }
  }
  CHECK(recved);
  return msg;
}

}  // anonymous namespace

// High-level receive.
zmq::message_t ZMQRecv(zmq::socket_t* sock, std::string* src_id) {
  zmq::message_t msg_src_id = ZMQRecvInternal(sock);
  if (src_id != nullptr) {
    *src_id = std::string(reinterpret_cast<const char*>(msg_src_id.data()),
        msg_src_id.size());
  }
  auto msg = ZMQRecvInternal(sock);
  // second frame is always empty.
  // Comment(wdai): This is to immitate REQ socket behavior, which adds an
  // empty frame.
  CHECK_EQ(0, msg.size());
  // This msg must be from a REQ socket, which prepends an empty frame. Read
  // one more frame.
  msg = ZMQRecvInternal(sock);
  CHECK_GT(msg.size(), 0) << "Empty msg.";
  return msg;
}

}   // zmq_util
}   // namespace hotbox
