// A simple HTTP echo server.
// Sends back a "Hello, World!", optionally followed by the data sent in in the body.

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#include <unistd.h>

const int kPort = 8080;
const int kMaxQueuedConnections = 1024;
const int kMaxInputPacketSize = 1024 * 1024;

const char* kHeadBodySeparator = "\r\n\r\n";

int main() {
  const int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    std::cerr << "Can't open socket, code " << sock << std::endl;
    return -1;
  }

  int just_one = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &just_one, sizeof(int));

  sockaddr_in addr_server;
  addr_server.sin_family = AF_INET;
  addr_server.sin_addr.s_addr = INADDR_ANY;
  addr_server.sin_port = htons(kPort);

  if (bind(sock, (sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
    close(sock);
    std::cerr << "Can't bind the socket." << std::endl;
    return -1;
  }

  char buffer[kMaxInputPacketSize + 1];

  listen(sock, kMaxQueuedConnections);

  while (true) {
    std::cout << "Accepting connections." << std::endl;
    sockaddr_in addr_client;
    socklen_t addr_client_length = sizeof(sockaddr_in);
    const int fd = accept(sock, (struct sockaddr*)&addr_client, &addr_client_length);

    std::cout << "Client connected." << std::endl;

    if (fd == -1) {
      std::cerr << "Can't accept the connection." << std::endl;
      continue;
    }

    // Note that this is a blocking call. It will keep all other requests waiting.
    const int length = read(fd, buffer, kMaxInputPacketSize);
    if (length < 0) {
      close(fd);
      std::cerr << "Can't read from the socket, code = " << length << std::endl;
      continue;
    }
    buffer[length] = '\0';

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
    write(fd, full_response.c_str(), full_response.length());

    close(fd);
  }
}
