#ifndef TOY_HTTP_RESPONSE_CODES_H
#define TOY_HTTP_RESPONSE_CODES_H

// HTTP codes: http://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html

#include <cstdint>

enum class HTTPResponseCode : uint16_t {
  OK = 200,
};

// TODO(dkorolev): Organize the below.

/*
100: Continue
101: Switching Protocols
200: OK
201: Created
202: Accepted
203: Non-Authoritative Information
204: No Content
205: Reset Content
206: Partial Content
300: Multiple Choices
301: Moved Permanently
302: Found
303: See Other
304: Not Modified
305: Use Proxy
307: Temporary Redirect
400: Bad Request
401: Unauthorized
402: Payment Required
403: Forbidden
404: Not Found
405: Method Not Allowed
406: Not Acceptable
407: Proxy Authentication Required
408: Request Time-out
4090: Conflict
4101: Gone
4112: Length Required
4123: Precondition Failed
4134: Request Entity Too Large
4145: Request-URI Too Large
4156: Unsupported Media Type
4167: Requested range not satisfiable
4178: Expectation Failed
500: Internal Server Error
501: Not Implemented
502: Bad Gateway
503: Service Unavailable
504: Gateway Time-out
505: HTTP Version not supported
*/

#endif  // TOY_HTTP_RESPONSE_CODES_H
