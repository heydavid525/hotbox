#include "zmq_util.hpp"
#include <glog/logging.h>

namespace mldb {
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
}

std::string Convert2ZmqId(const std::string& id_str) {
  auto copy_str = id_str;
  // Turn leftmost bit to 1.
  copy_str[0] = copy_str[0] | 0x80;
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
  size_t nbytes;
  try {
    nbytes = sock->send(data, len, flag);
  } catch (zmq::error_t &e) {
    switch (e.num()) {
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
        return false;
    }
  }
  return nbytes == len;
}

}  // anonymous namespace


// High-level send.
bool ZMQSend(zmq::socket_t* sock, const std::string& dst_id,
    const std::string& data) {
  LOG(INFO) << "dst_id: " << dst_id;
  bool zid_sent = ZMQSendInternal(sock, dst_id.c_str(), dst_id.size(),
      ZMQ_SNDMORE);
  LOG(INFO) << "zid_sent: " << zid_sent;
  return zid_sent && ZMQSendInternal(sock, data.c_str(), data.size());
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
  return ZMQRecvInternal(sock);
}

}   // zmq_util
}   // namespace mldb
