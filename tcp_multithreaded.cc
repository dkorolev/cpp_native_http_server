// A simple multithreaded TCP server.

/*
# To test:
telnet 127.0.0.1 8080
*/

#include <thread>

#include "posix_tcp_server.h"

const int kPort = 8080;

int main() {
  Socket s(kPort);
  while (true) {
    std::thread([](Connection c) {
                  try {
                    c.BlockingWrite("BAZINGA\n");
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    c.BlockingWrite("DONE\n");
                  } catch (NetworkException&) {
                  }
                },
                std::move(s.Accept())).detach();
  }
}
