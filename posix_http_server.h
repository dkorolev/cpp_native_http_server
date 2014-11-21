#ifndef TOY_POSIX_HTTP_SERVER_H
#define TOY_POSIX_HTTP_SERVER_H

// HTTP message: http://www.w3.org/Protocols/rfc2616/rfc2616.html

#include <map>
#include <string>
#include <vector>

#include "exceptions.h"
#include "posix_tcp_server.h"
#include "http_response_codes.h"

typedef std::vector<std::pair<std::string, std::string>> HTTPHeadersType;

class HTTPHeaderParser {
 public:
  const std::string& Method() const {
    return method_;
  }

  const std::string& URL() const {
    return url_;
  }

  const bool HasBody() const {
    return content_offset_ != static_cast<size_t>(-1) && content_length_ != static_cast<size_t>(-1);
  }

  const std::string Body() const {
    if (HasBody()) {
      return std::string(&buffer_[content_offset_], &buffer_[content_offset_] + content_length_);
    } else {
      throw HTTPNoBodyProvidedException();
    }
  }

  const char* const BodyAsNonCopiedBuffer() const {
    if (HasBody()) {
      return &buffer_[content_offset_];
    } else {
      throw HTTPNoBodyProvidedException();
    }
  }

  const size_t BodyLength() const {
    if (HasBody()) {
      return content_length_;
    } else {
      throw HTTPNoBodyProvidedException();
    }
  }

 protected:
  // Parses HTTP headers. Extracts method, URL, and body, if provided.
  // Can be statically overridden by providing a different templated class as a parameter for GenericHTTPConnection.
  void ParseHTTPHeader(const GenericConnection& c) {
    // `buffer_` stores all the stream of data read from the socket, headers followed by optional body.
    size_t current_line_offset = 0;

    // `first_line_parsed` denotes whether the line being parsed is the first one, with method and URL.
    bool first_line_parsed = false;

    // `offset` is the number of bytes read so far.
    // `length_cap` is infinity first (size_t is unsigned), and it changes/ to the absolute offset
    // of the end of HTTP body in the buffer_, once `Content-Length` and two consecutive CRLS have been seen.
    size_t offset = 0;
    size_t length_cap = static_cast<size_t>(-1);

    while (offset < length_cap) {
      size_t chunk;
      size_t read_count;
      // Use `- offset - 1` instead of just `- offset` to leave room for the '\0'.
      while (chunk = buffer_.size() - offset - 1,
             read_count = c.BlockingRead(&buffer_[offset], chunk),
             offset += read_count,
             read_count == chunk) {
        buffer_.resize(buffer_.size() * kHTTPBufferGrowthFactor);
      }
      buffer_[offset] = '\0';
      char* p = &buffer_[current_line_offset];
      char* current_line = p;
      while ((p = strstr(current_line, kCRLF))) {
        *p = '\0';
        if (!first_line_parsed) {
          if (*current_line) {
            // It's recommended by W3 to wait for the first line ignoring prior CRLF-s.
            char* p1 = current_line;
            char* p2 = strstr(p1, " ");
            if (p2) {
              *p2 = '\0';
              ++p2;
              method_ = p1;
              char* p3 = strstr(p2, " ");
              if (p3) {
                *p3 = '\0';
              }
              url_ = p2;
            }
            first_line_parsed = true;
          }
        } else {
          if (*current_line) {
            char* p = strstr(current_line, kHeaderKeyValueSeparator);
            if (p) {
              *p = '\0';
              const char* const key = current_line;
              const char* const value = p + kHeaderKeyValueSeparatorLength;
              OnHeader(key, value);
              if (!strcmp(key, kContentLengthHeaderKey)) {
                content_length_ = static_cast<size_t>(atoi(value));
              }
            }
          } else {
            // HTTP body starts right after this last CRLF.
            content_offset_ = current_line + kCRLFLength - &buffer_[0];
            // Only accept HTTP body if Content-Length has been set; ignore it otherwise.
            if (content_length_ != static_cast<size_t>(-1)) {
              length_cap = content_offset_ + content_length_;
            } else {
              length_cap = content_offset_;
            }
          }
        }
        current_line = p + 2;
      }
      current_line_offset = current_line - &buffer_[0];
    }
  }

  // Can be statically overridden by proviging a different templated class to GenericHTTPConnection.
  void OnHeader(const char* key, const char* value) {
    headers_[key] = value;
  }

 private:
  // HTTP buffer is used to store HTTP headers, and, if provided, HTTP body.
  const size_t kHTTPBufferInitialSize = 1600;
  const double kHTTPBufferGrowthFactor = 1.95;

  // HTTP constants to parse the header and extract method, URL, headers and body.
  const char* const kCRLF = "\r\n";
  const size_t kCRLFLength = strlen(kCRLF);
  const char* const kHeaderKeyValueSeparator = ": ";
  const size_t kHeaderKeyValueSeparatorLength = strlen(kHeaderKeyValueSeparator);
  const char* const kContentLengthHeaderKey = "Content-Length";

 private:
  std::string method_;
  std::string url_;
  std::map<std::string, std::string> headers_;
  std::vector<char> buffer_ = std::vector<char>(kHTTPBufferInitialSize);
  size_t content_offset_ = static_cast<size_t>(-1);
  size_t content_length_ = static_cast<size_t>(-1);
};

template <typename HEADER_PARSER = HTTPHeaderParser>
class GenericHTTPConnection final : public GenericConnection, public HEADER_PARSER {
 public:
  typedef HEADER_PARSER T_HEADER_PARSER;

  GenericHTTPConnection(GenericConnection&& c) : GenericConnection(std::move(c)), T_HEADER_PARSER() {
    T_HEADER_PARSER::ParseHTTPHeader(*this);
  }

  GenericHTTPConnection(GenericHTTPConnection&& c) : GenericConnection(std::move(c)), T_HEADER_PARSER() {
    T_HEADER_PARSER::ParseHTTPHeader(*this);
  }

  // TODO(dkorolev): Add Content-Type, with a default value.
  template <typename T>
  typename std::enable_if<sizeof(typename T::value_type) == 1>::type SendHTTPResponse(
      const T& begin,
      const T& end,
      HTTPResponseCode code = HTTPResponseCode::OK,
      HTTPHeadersType extra_headers = HTTPHeadersType()) {
    if (responded_) {
      throw HTTPAttemptedToRespondTwiceException();
    }
    responded_ = true;
    // TODO(dkorolev): Send HTTP response code.
    // TODO(dkorolev): Send HTTP headers.
    BlockingWrite(begin, end);
  }

  template <typename T>
  typename std::enable_if<sizeof(typename T::value_type) == 1>::type SendHTTPResponse(
      const T& container,
      HTTPResponseCode code = HTTPResponseCode::OK,
      HTTPHeadersType extra_headers = HTTPHeadersType()) {
    SendHTTPResponse(container.begin(), container.end(), code);
  }

 private:
  bool responded_ = false;

  GenericHTTPConnection(const GenericHTTPConnection&) = delete;
  void operator=(const GenericHTTPConnection&) = delete;
  void operator=(GenericHTTPConnection&&) = delete;
};

// Default HTTPConnection parses URL, method, and body for requests with Content-Length.
typedef GenericHTTPConnection<HTTPHeaderParser> HTTPConnection;

#endif  // TOY_POSIX_HTTP_SERVER_H
