// A simple multithreaded HTTP server.

/*
# To test:
curl localhost:8080
curl -d DATA localhost:8080
*/

#include <thread>

#include "posix_socket.h"

const int kPort = 8080;

int main() {
  Socket s(kPort);
  while (true) {
    std::thread([](HTTPConnection c) {
                  try {
                    c.BlockingWrite("BAZINGA(" + c.Body() + ")\n");
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    c.BlockingWrite("DONE\n");
                  } catch (NetworkException&) {
                  }
                },
                std::move(Connection(s.Accept()))).detach();
  }
}
