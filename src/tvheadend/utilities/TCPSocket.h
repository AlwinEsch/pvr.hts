/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "asio.hpp"

#include "Logger.h"

#include <chrono>

namespace tvheadend
{
namespace utilities
{

class TCPSocket
{
public:
  TCPSocket() = delete;
  TCPSocket(const std::string& host, uint16_t port) : m_host(host), m_port(port) {}

  virtual ~TCPSocket() { Close(); }

  bool Open(uint64_t iTimeoutMs)
  {
    auto endpoints = asio::ip::tcp::resolver(m_context).resolve(m_host, std::to_string(m_port));

    std::error_code error;
    asio::async_connect(
        m_socket, endpoints,
        [&](const std::error_code& result_error, const asio::ip::tcp::endpoint&) {
          error = result_error;
        });

    if (error)
      Logger::Log(LogLevel::LEVEL_ERROR, "failed to open socket (%s)", error.message().c_str());
    else
      RunFor(iTimeoutMs);

    return !error;
  }

  void Close() { m_socket.close(); }

  ssize_t Read(void* data, size_t len, uint64_t iTimeoutMs = 0)
  {
    std::error_code error;
    ssize_t n = -1;
    asio::async_read(m_socket, asio::buffer(data, len),
                     [&](const std::error_code& result_error, std::size_t result_n) {
                       n = result_n;
                       error = result_error;
                     });

    if (error)
      Logger::Log(LogLevel::LEVEL_ERROR, "failed to read from socket (%s)",
                  error.message().c_str());
    else
      RunFor(iTimeoutMs);

    return !error ? n : -1;
  }

  ssize_t Write(void* data, size_t len, uint64_t iTimeoutMs)
  {
    std::error_code error;
    ssize_t n = -1;
    asio::async_write(m_socket, asio::buffer(data, len),
                      [&](const std::error_code& result_error, std::size_t result_n) {
                        n = result_n;
                        error = result_error;
                      });

    if (error)
      Logger::Log(LogLevel::LEVEL_ERROR, "failed to write to socket (%s)",
                  error.message().c_str());
    else
      RunFor(iTimeoutMs);

    return !error ? n : -1;
  }

private:
  void RunFor(uint64_t iTimeoutMs)
  {
    m_context.restart();

    if (iTimeoutMs == 0)
    {
      // block until all work has finished
      m_context.run();
    }
    else
    {
      // block until all work has finished our time is up
      m_context.run_for(std::chrono::milliseconds(iTimeoutMs));
    }

    if (!m_context.stopped())
    {
      m_context.restart();
      m_context.run();
    }
  }

  std::string m_host;
  uint16_t m_port = 0;

  asio::io_context m_context;
  asio::ip::tcp::socket m_socket{m_context};
};

} // namespace utilities
} // namespace tvheadend
