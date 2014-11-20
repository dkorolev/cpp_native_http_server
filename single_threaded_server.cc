// A simple HTTP echo server.
// Sends back a "Hello, World!", optionally followed by the data sent in in the body.

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <unistd.h>

const int kPort = 8080;
const size_t kMaxQueuedConnections = 1024;
const size_t kMaxInputPacketSize = 1024 * 1024;

const char* kHeadBodySeparator = "\r\n\r\n";  // TODO(dkorolev): Go through the RFC for HTTP and confirm this magic.

struct NetworkException : std::exception {};
struct SocketException : NetworkException {};
struct SocketCreateException : SocketException {};
struct SocketBindException : SocketException {};
struct SocketListenException : SocketException {};
struct SocketAcceptException : SocketException {};
struct SocketReadException : SocketException {};
struct SocketWriteException : SocketException {};
struct SocketCouldNotWriteEverythingException : SocketWriteException {};

class Connection {
 public:
  explicit Connection(const int fd) : fd_(fd) {
    if (fd_ == -1) {
      throw SocketAcceptException();
    }
    std::cout << "Client connected." << std::endl;
  }

  Connection(Connection&& rhs) : fd_(-1) {
    std::swap(fd_, rhs.fd_);
  }

  ~Connection() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  template <typename T>
  size_t BlockingRead(T* buffer, size_t max_size = kMaxInputPacketSize) {
    const int read_length = read(fd_, reinterpret_cast<void*>(buffer), kMaxInputPacketSize * sizeof(T));
    if (read_length < 0) {
      throw SocketReadException();
    }
    return static_cast<size_t>(read_length);
  }

  void BlockingWrite(const void* buffer, size_t write_length) {
    const int result = write(fd_, buffer, write_length);
    if (result < 0) {
      throw SocketWriteException();
    } else if (result != static_cast<int>(write_length)) {
      throw SocketCouldNotWriteEverythingException();
    }
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

  Connection(const Connection&) = delete;
  void operator=(const Connection&) = delete;
  void operator=(Connection&&) = delete;
};

class Socket {
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

  Connection Accept() {
    sockaddr_in addr_client;
    socklen_t addr_client_length = sizeof(sockaddr_in);
    return Connection(accept(socket_, (struct sockaddr*)&addr_client, &addr_client_length));
  }

 private:
  const int socket_;

  Socket(const Socket&) = delete;
  Socket(Socket&&) = delete;
  void operator=(const Socket&) = delete;
  void operator=(Socket&&) = delete;
};

int main() {
  Socket s(kPort);

  char buffer[kMaxInputPacketSize + 1];

  while (true) {
    std::cout << "Accepting connections." << std::endl;

    Connection c = s.Accept();

    try {
      buffer[c.BlockingRead(buffer, kMaxInputPacketSize)] = '\0';
    } catch (SocketReadException&) {
      continue;
    }

    const char* data = strstr(buffer, kHeadBodySeparator);
    if (data) {
      data += strlen(kHeadBodySeparator);
      if (!*data) {
        data = nullptr;
      }
    }

    std::ostringstream os;
    std::string response = "Hello, World!\n";
    if (data) {
      response += data;
      response += '\n';
    }

    os << "HTTP/1.1 200 OK\n";
    os << "Content-type: text/html\n";
    os << "Content-length: " << response.length() << "\n";
    os << "\n";
    os << response;

    const std::string full_response = os.str();
    try {
      c.BlockingWrite(full_response);
    } catch (SocketCouldNotWriteEverythingException&) {
    }
  }
}
