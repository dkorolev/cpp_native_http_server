#ifndef TOY_EXCEPTIONS_H
#define TOY_EXCEPTIONS_H

#include <exception>

struct NetworkException : std::exception {};

struct SocketException : NetworkException {};

struct SocketCreateException : SocketException {};
struct SocketBindException : SocketException {};
struct SocketListenException : SocketException {};
struct SocketAcceptException : SocketException {};
struct SocketReadException : SocketException {};
struct SocketWriteException : SocketException {};
struct SocketCouldNotWriteEverythingException : SocketWriteException {};

struct HTTPException : NetworkException {};
struct HTTPNoBodyProvidedException : HTTPException {};
struct HTTPAttemptedToRespondTwiceException : HTTPException {};

#endif  // TOY_EXCEPTIONS_H
