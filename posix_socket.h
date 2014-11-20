#ifndef TOY_POSIX_SOCKET_H
#define TOY_POSIX_SOCKET_H

#include "exceptions.h"

#include <cassert>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <unistd.h>

const size_t kDefaultMaxLengthToReceive = 1024 * 1024;
const size_t kMaxQueuedConnections = 1024;

const size_t kInitialHTTPHeaderSize = 1600;
const double kHTTPHeaderSizeGrowthFactor = 1.95;

// The separator is a blank line, via http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
const char* kHTTPHeadBodySeparator = "\r\n\r\n";

class GenericConnection {
 public:
  explicit GenericConnection(const int fd) : fd_(fd) {
  }

  GenericConnection(GenericConnection&& rhs) : fd_(-1) {
    std::swap(fd_, rhs.fd_);
  }

  ~GenericConnection() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  template <typename T>
  size_t BlockingRead(T* buffer, size_t max_length = kDefaultMaxLengthToReceive) {
    const int read_length = read(fd_, reinterpret_cast<void*>(buffer), max_length * sizeof(T));
    if (read_length < 0) {
      throw SocketReadException();
    }
    return static_cast<size_t>(read_length);
  }

  void BlockingWrite(const void* buffer, size_t write_length) {
    assert(buffer);
    const int result = write(fd_, buffer, write_length);
    if (result < 0) {
      throw SocketWriteException();
    } else if (result != static_cast<int>(write_length)) {
      throw SocketCouldNotWriteEverythingException();
    }
  }

  void BlockingWrite(const char* s) {
    assert(s);
    BlockingWrite(s, strlen(s));
  }

  template <typename T>
  void BlockingWrite(const T begin, const T end) {
    BlockingWrite(&(*begin), (end - begin) * sizeof(typename T::value_type));
  }

  template <typename T>
  void BlockingWrite(const T& container) {
    BlockingWrite(container.begin(), container.end());
  }

 private:
  int fd_;

  GenericConnection(const GenericConnection&) = delete;
  void operator=(const GenericConnection&) = delete;
  void operator=(GenericConnection&&) = delete;
};

class Connection final : public GenericConnection {
 public:
  Connection(GenericConnection&& c) : GenericConnection(std::move(c)) {
  }

  Connection(Connection&& c) : GenericConnection(std::move(c)) {
  }
};

// TODO(dkorolev): Parse the requested URL as well, not just the body.
class HTTPConnection final : public GenericConnection {
 public:
  HTTPConnection(GenericConnection&& c) : GenericConnection(std::move(c)) {
    ParseHTTPHeaders();
  }

  HTTPConnection(HTTPConnection&& c) : GenericConnection(std::move(c)) {
    ParseHTTPHeaders();
  }

  const std::string& Body() const {
    return body_;
  }

 protected:
  void ParseHTTPHeaders() {
    std::vector<char> header(kInitialHTTPHeaderSize);
    size_t size = 0;
    size_t chunk;
    while (chunk = header.size() - size - 1, BlockingRead(&header[size], chunk) == chunk) {
      header.resize(header.size() * kHTTPHeaderSizeGrowthFactor);
    }
    header[size + chunk] = '\0';

    const char* data = strstr(&header[0], kHTTPHeadBodySeparator);
    if (data) {
      data += strlen(kHTTPHeadBodySeparator);
      if (*data) {
        body_ = data;
      }
    }
  }

 private:
  std::string body_;

  HTTPConnection(const HTTPConnection&) = delete;
  void operator=(const HTTPConnection&) = delete;
  void operator=(HTTPConnection&&) = delete;
};

class Socket final {
 public:
  explicit Socket(int port, int max_connections = kMaxQueuedConnections) : socket_(socket(AF_INET, SOCK_STREAM, 0)) {
    if (socket_ < 0) {
      throw SocketCreateException();
    }

    int just_one = 1;
    setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &just_one, sizeof(int));

    sockaddr_in addr_server;
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = INADDR_ANY;
    addr_server.sin_port = htons(port);

    if (bind(socket_, (sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
      close(socket_);
      throw SocketBindException();
    }

    if (listen(socket_, max_connections)) {
      close(socket_);
      throw SocketListenException();
    }
  }

  ~Socket() {
    close(socket_);
  }

  GenericConnection Accept() const {
    sockaddr_in addr_client;
    socklen_t addr_client_length = sizeof(sockaddr_in);
    const int fd = accept(socket_, (struct sockaddr*)&addr_client, &addr_client_length);
    if (fd == -1) {
      throw SocketAcceptException();
    }
    return GenericConnection(fd);
  }

 private:
  const int socket_;

  Socket(const Socket&) = delete;
  Socket(Socket&&) = delete;
  void operator=(const Socket&) = delete;
  void operator=(Socket&&) = delete;
};

#endif  // TOY_POSIX_SOCKET_H
