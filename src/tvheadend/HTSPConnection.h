/*
 *  Copyright (C) 2017-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

extern "C"
{
#include "libhts/htsmsg.h"
}

#include "kodi/addon-instance/pvr/General.h"
#include "p8-platform/threads/mutex.h"
#include "p8-platform/threads/threads.h"

#include <asio.hpp>
#include <chrono>

namespace tvheadend
{

class HTSPRegister;
class HTSPResponse;
class IHTSPConnectionListener;

typedef std::map<uint32_t, HTSPResponse*> HTSPResponseList;

/*
 * HTSP Connection
 */
class HTSPConnection : public P8PLATFORM::CThread
{
public:
  HTSPConnection(IHTSPConnectionListener& connListener);
  ~HTSPConnection() override;

  void Start();
  void Stop();
  void Disconnect();

  bool SendMessage0(const char* method, htsmsg_t* m);
  htsmsg_t* SendAndWait0(const char* method, htsmsg_t* m, int iResponseTimeout = -1);
  htsmsg_t* SendAndWait(const char* method, htsmsg_t* m, int iResponseTimeout = -1);

  int GetProtocol() const;

  std::string GetWebURL(const char* fmt, ...) const;

  std::string GetServerName() const;
  std::string GetServerVersion() const;
  std::string GetServerString() const;

  bool HasCapability(const std::string& capability) const;

  inline P8PLATFORM::CMutex& Mutex() { return m_mutex; }

  void OnSleep();
  void OnWake();

private:
  // CThread iplementation
  void* Process() override;

  void Register();
  bool ReadMessage();
  bool SendHello();
  bool SendAuth(const std::string& u, const std::string& p);

  void SetState(PVR_CONNECTION_STATE state);
  bool WaitForConnection();

  /*
   * HTSP Connection registration thread
   */
  class HTSPRegister : public P8PLATFORM::CThread
  {
  public:
    HTSPRegister(HTSPConnection* conn) : m_conn(conn) {}

    ~HTSPRegister() override { StopThread(0); }

  private:
    // CThread implementation
    void* Process() override
    {
      m_conn->Register();
      return nullptr;
    }

    HTSPConnection* m_conn;
  };

  bool Run(std::chrono::steady_clock::duration timeout)
  {
    // Restart the io_context, as it may have been left in the "stopped" state
    // by a previous operation.
    m_ioContext.restart();

    // Block until the asynchronous operation has completed, or timed out. If
    // the pending asynchronous operation is a composed operation, the deadline
    // applies to the entire operation, rather than individual operations on
    // the socket.
    m_ioContext.run_for(timeout);

    // If the asynchronous operation completed successfully then the io_context
    // would have been stopped due to running out of work. If it was not
    // stopped, then the io_context::run_for call must have timed out.
    return m_ioContext.stopped();
  }

  IHTSPConnectionListener& m_connListener;
  asio::io_context m_ioContext;
  asio::ip::tcp::socket m_ioSocket{m_ioContext};
  mutable P8PLATFORM::CMutex m_mutex;
  HTSPRegister* m_regThread;
  P8PLATFORM::CCondition<volatile bool> m_regCond;
  bool m_ready;
  uint32_t m_seq;
  std::string m_serverName;
  std::string m_serverVersion;
  int m_htspVersion;
  std::string m_webRoot;
  void* m_challenge;
  int m_challengeLen;

  HTSPResponseList m_messages;
  std::vector<std::string> m_capabilities;

  bool m_suspended;
  PVR_CONNECTION_STATE m_state;
};

} // namespace tvheadend
