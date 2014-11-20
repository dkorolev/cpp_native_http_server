// A simple single-threaded HTTP server.

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
    HTTPConnection c(s.Accept());
    try {
      c.BlockingWrite("BAZINGA(" + c.Body() + ")\n");
      std::this_thread::sleep_for(std::chrono::seconds(10));
      c.BlockingWrite("DONE\n");
    } catch (NetworkException&) {
    }
  }
}
