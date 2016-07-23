#pragma once

namespace hotbox {

class HBProxyClient {
public:
  HBProxyClient();

  // Create a session. HBClient must outlive the created Session.
  Session CreateSession(const SessionOptions& session_options) noexcept;

  Session* CreateSessionPtr(const SessionOptions& session_options) noexcept;

private:
  WarpClient warp_client_;
};

}  // namespace hotbox
