// A simple multithreaded HTTP server.

/*
# To test:
curl localhost:8080
curl -d DATA localhost:8080
(echo -e "GET /\n\n" ; sleep 1) | telnet 0.0.0.0 8080  # telnet converts `\n` into `\r\n`.
(echo -e "GET /\nContent-Length: 6\n\nPASSED; Ignored." ; sleep 1) | telnet 0.0.0.0 8080
*/

#include <iostream>
#include <sstream>
#include <thread>

#include "posix_http_server.h"

const int kPort = 8080;

int main() {
  Socket s(kPort);
  while (true) {
    std::thread([](HTTPConnection c) {
                  std::ostringstream os;
                  os << "BAZINGA\n" << c.Method() << "(" << c.URL() << ")\n";
                  if (c.HasBody()) {
                    os << c.Body() << '\n';
                  }
                  c.SendHTTPResponse(os.str(), HTTPResponseCode::OK);
                  std::cout << "Waiting for 10 seconds in the serving thread." << std::endl;
                  std::this_thread::sleep_for(std::chrono::seconds(10));
                  std::cout << "Terminating the serving thread." << std::endl;
                },
                std::move(s.Accept())).detach();
  }
}
