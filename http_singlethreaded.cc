// A simple single-threaded HTTP server.

/*
# To test:
curl localhost:8080
curl -d DATA localhost:8080
*/

#include <iostream>
#include <sstream>
#include <thread>

#include "posix_socket.h"

const int kPort = 8080;

int main() {
  Socket s(kPort);
  while (true) {
    HTTPConnection c(s.Accept());
    try {
      std::ostringstream os;
      os << "BAZINGA\n" << c.Method() << ' ' << c.URL() << '\n';
      if (c.HasBody()) {
        os << c.Body() << '\n';
      }
      c.SendHTTPResponse(os.str(), HTTPResponseCode::OK);
      std::cout << "Waiting for 10 seconds in the serving thread." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(10));
      std::cout << "Terminating the serving thread." << std::endl;
    } catch (NetworkException&) {
    }
  }
}
