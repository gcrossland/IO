#include "io_socket.hpp"
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

namespace io { namespace socket {

using core::u8string;
using std::tuple;
using std::vector;
using core::PlainException;
using std::move;
using std::get;
using core::unsign;
using core::numeric_limits;

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
DC();

} namespace file {
core::u8string createStrerror (int errnum, const char8_t *prefix = u8(" ("), const char8_t *suffix = u8(")"));
} namespace socket {
using io::file::createStrerror;

// TODO transplant + handle text properly
template<typename _i> void stringise (u8string &r_out, _i value) {
  auto s = std::to_string(value);
  r_out.append(reinterpret_cast<const char8_t *>(s.data()), s.size());
}

u8string createGaiStrerror (int errnum, const char8_t *prefix = u8(" ("), const char8_t *suffix = u8(")")) {
  u8string msg;
  if (errnum) {
    // TODO handle text properly (encoding, initial letter capitalisation)
    // TODO transplant this and core::createExceptionMessage() to somewhere after local encoding handling (umain, setlocale calling etc.)
    const char *s = gai_strerror(errnum);

    if (s && s[0] != u8("\0")[0]) {
      msg += prefix;
      msg += reinterpret_cast<const char8_t *>(s);
      msg += suffix;
    }
  }
  return msg;
}

TcpSocketAddress::TcpSocketAddress (const sockaddr_in &socketAddr4) {
  _.socketAddr4 = socketAddr4;
  DPRE(getFamily() == AF_INET);
  DI(
    u8string rep = u8("<<");
    getSocketAddress(rep);
    rep.append(u8(">>"));
  )
  DW(, "made TcpSocketAddress - IPv4 ", rep.c_str());
}

TcpSocketAddress::TcpSocketAddress (const sockaddr_in6 &socketAddr6) {
  _.socketAddr6 = socketAddr6;
  DPRE(getFamily() == AF_INET6);
  DI(
    u8string rep = u8("<<");
    getSocketAddress(rep);
    rep.append(u8(">>"));
  )
  DW(, "made TcpSocketAddress - IPv6 ", rep.c_str());
}

sa_family_t TcpSocketAddress::getFamily () const noexcept {
  DA(static_cast<const void *>(&_) == static_cast<const void *>(&_.socketAddr4));
  DA(static_cast<const void *>(&_) == static_cast<const void *>(&_.socketAddr6));
  return reinterpret_cast<const sockaddr *>(&_)->sa_family;
}

tuple<const sockaddr *, socklen_t> TcpSocketAddress::getSocketAddress () const noexcept {
  return tuple<const sockaddr *, socklen_t>(reinterpret_cast<const sockaddr *>(&_), getFamily() == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
}

void TcpSocketAddress::getSocketAddress (u8string &r_r) const noexcept {
  sa_family_t family = getFamily();
  const void *src;
  size_t repMaxSize;
  iu16f port;
  switch (family) {
    case AF_INET:
      src = &_.socketAddr4.sin_addr;
      repMaxSize = INET_ADDRSTRLEN;
      port = ntohs(_.socketAddr4.sin_port);
      break;
    case AF_INET6:
      src = &_.socketAddr6.sin6_addr;
      repMaxSize = INET6_ADDRSTRLEN;
      port = ntohs(_.socketAddr6.sin6_port);
      break;
    default:
      DA(false);
      src = nullptr;
      repMaxSize = 0;
      port = 0;
  }

  size_t rSize0 = r_r.size();
  r_r.append_any(repMaxSize);
  char8_t *repI = r_r.data() + rSize0;

  const char *r = inet_ntop(family, src, reinterpret_cast<char *>(repI), repMaxSize);
  if (!r) {
    r_r.erase(rSize0);
    r_r.append(u8("????"));
  } else {
    r_r.erase(rSize0 + strlen(reinterpret_cast<char *>(repI)));
  }

  r_r.append(u8(":"));
  stringise(r_r, port);
}

addrinfo EMPTY_ADDRINFO;

void TcpSocketAddress::get (vector<TcpSocketAddress> &r_addrs, const char8_t *nodeName, iu16f port) {
  addrinfo hints = EMPTY_ADDRINFO;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = TcpSocketAddress::TYPE;
  hints.ai_protocol = TcpSocketAddress::PROTOCOL;
  // TODO use AI_ADDRCONFIG on platforms using the RFC 2553-style omission only of DNS-found results (vs. RFC 3493)
  hints.ai_flags = 0;
  if (!nodeName) {
    hints.ai_flags |= AI_PASSIVE;
  }

  addrinfo *addrs;
  DW(, "resolving '", nodeName ? nodeName : u8("<binding wildcard address>"), "':");
  int r = getaddrinfo(nodeName ? reinterpret_cast<const char *>(nodeName) : nullptr, nodeName ? nullptr : "0", &hints, &addrs);
  if (r != 0) {
    u8string msg = nodeName ?
      u8string(u8("failed to get IP address for '")) + nodeName + u8("'") :
      u8string(u8("failed to get any local IP addresses"))
    ;
    throw PlainException(move(msg) + createGaiStrerror(r));
  }
  finally([&] () {
    DW(, "freeing addrinfos");
    freeaddrinfo(addrs);
  });

  DW(, "resolution got:");
  for (addrinfo *i = addrs; i; i = i->ai_next) {
    DW(, "  entry:");
    DW(, "    family ", i->ai_family);
    DW(, "    socktype ", i->ai_socktype);
    DW(, "    protocol ", i->ai_protocol);
    DW(, "    addrlen ", i->ai_addrlen);
    if (i->ai_socktype != TcpSocketAddress::TYPE || i->ai_protocol != TcpSocketAddress::PROTOCOL) {
      continue;
    }
    DW(, "    is tcp (and stream)");

    switch (i->ai_family) {
      case AF_INET: {
        DW(, "    is ipv4");
        sockaddr_in &socketAddr = *reinterpret_cast<sockaddr_in *>(i->ai_addr);
        DA(i->ai_addrlen == sizeof(socketAddr));
        socketAddr.sin_port = htons(port);
        r_addrs.push_back(TcpSocketAddress(socketAddr));
      } break;
      case AF_INET6: {
        DW(, "    is ipv6");
        sockaddr_in6 &socketAddr = *reinterpret_cast<sockaddr_in6 *>(i->ai_addr);
        DA(i->ai_addrlen == sizeof(socketAddr));
        socketAddr.sin6_port = htons(port);
        r_addrs.push_back(TcpSocketAddress(socketAddr));
      } break;
      default:
        DW(, "    not ip");
        break;
    }
  }
}

void TcpSocketAddress::get (vector<TcpSocketAddress> &r_addrs, const u8string &nodeName, iu16f port) {
  get(r_addrs, nodeName.c_str(), port);
}

void TcpSocketAddress::get (vector<TcpSocketAddress> &r_addrs, const u8string &nodeNameAndPort) {
  size_t i = nodeNameAndPort.rfind(u8(":")[0]);
  if (i == u8string::npos) {
    throw PlainException(u8string(u8("'")) + nodeNameAndPort + u8("' is not of the form host-name:port-number"));
  }

  const char8_t *begin = nodeNameAndPort.c_str();
  const char8_t *end = begin + nodeNameAndPort.size();
  char8_t *numberEnd;
  long number = strtol(reinterpret_cast<const char *>(begin + i + 1), reinterpret_cast<char **>(&numberEnd), 10);
  if (numberEnd != end || number < 0 || number > 65535) {
    throw PlainException(u8string(u8("'")) + nodeNameAndPort + u8("' does not contain a valid port number"));
  }
  auto port = static_cast<iu16f>(number);

  char8_t nodeName[i + 1];
  memcpy(nodeName, begin, i);
  nodeName[i] = u8("\0")[0];
  get(r_addrs, nodeName, port);
}

void TcpSocketAddress::get (vector<TcpSocketAddress> &r_addrs, iu16f port) {
  get(r_addrs, nullptr, port);
}

Socket::Socket (decltype(s) s) : s(s) {
  DPRE(s != -1);
}

Socket::Socket (sa_family_t family, int type, int protocol) {
  s = ::socket(family, type, protocol);
  if (s == -1) {
    throw PlainException(u8string(u8("failed to create a network socket")) + createStrerror(errno));
  }
}

Socket::Socket (const TcpSocketAddress &addr) : Socket(addr.getFamily(), TcpSocketAddress::TYPE, TcpSocketAddress::PROTOCOL) {
}

Socket::Socket (Socket &&o) noexcept : s(-1) {
  *this = move(o);
}

Socket &Socket::operator= (Socket &&o) noexcept {
  if (this != &o) {
    if (s != -1) {
      ::close(s);
    }
    s = o.s;
    o.s = -1;
  }
  return *this;
}

Socket::~Socket () noexcept {
  if (s != -1) {
    ::close(s);
  }
}

void Socket::bind (const TcpSocketAddress &addr) {
  DPRE(s != -1);
  tuple<const sockaddr *, socklen_t> o = addr.getSocketAddress();
  int r = ::bind(s, get<0>(o), get<1>(o));
  if (r == -1) {
    u8string msg = u8("failed to associate a network socket with the address at which it was to listen, ");
    addr.getSocketAddress(msg);
    throw PlainException(move(msg) + createStrerror(errno));
  }
}

void Socket::listen (iu listenBacklog) {
  DPRE(s != -1);
  auto maxListenBacklog = unsign(numeric_limits<int>::max());
  if (listenBacklog > maxListenBacklog) {
    listenBacklog = maxListenBacklog;
  }

  int r = ::listen(s, static_cast<int>(listenBacklog));
  if (r == -1) {
    throw PlainException(u8string(u8("failed to make a network socket listen")) + createStrerror(errno));
  }
}

Socket Socket::accept () {
  DPRE(s != -1);
  decltype(s) s0 = ::accept(s, nullptr, 0);
  if (s0 == -1) {
    throw PlainException(u8string(u8("failed to accept connections to a listening network socket")) + createStrerror(errno));
  }
  return Socket(s0);
}

void Socket::connect (const TcpSocketAddress &addr) {
  DPRE(s != -1);
  tuple<const sockaddr *, socklen_t> o = addr.getSocketAddress();
  int r = ::connect(s, get<0>(o), get<1>(o));
  if (r == -1) {
    u8string msg = u8("failed to connect a network socket to ");
    addr.getSocketAddress(msg);
    throw PlainException(move(msg) + createStrerror(errno));
  }
}

void Socket::setOptions (bool keepalive) {
  DPRE(s != -1);
  {
    int optval = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
  }
  {
    int optval = keepalive;
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
  }
}

ssize_t Socket::recv (void *buf, size_t len) {
  DPRE(s != -1);
  ssize_t r = ::recv(s, buf, len, 0);
  if (r == -1) {
    throw PlainException(u8string(u8("failed to read from a network socket")) + createStrerror(errno));
  }
  return r;
}

ssize_t Socket::send (const void *buf, size_t len) {
  DPRE(s != -1);
  ssize_t r = ::send(s, buf, len, 0);
  if (r == -1) {
    throw PlainException(u8string(u8("failed to write to a network socket")) + createStrerror(errno));
  }
  return r;
}

void Socket::shutdown (int how) {
  DPRE(s != -1);
  ::shutdown(s, how);
}

void Socket::close () {
  if (closed()) {
    return;
  }

  int r = ::close(s);
  s = -1;
  if (r == -1) {
    throw PlainException(u8string(u8("failed to close a network socket")) + createStrerror(errno));
  }
}

bool Socket::closed () const noexcept {
  return s == -1;
}

TcpSocketStream::TcpSocketStream (Socket &&socket) : socket(move(socket)) {
}

TcpSocketStream::TcpSocketStream (const TcpSocketAddress &targetAddr, bool keepalive) : socket(targetAddr) {
  socket.connect(targetAddr);
  socket.setOptions(keepalive);
}

size_t TcpSocketStream::read (iu8f *b, size_t s) {
  DPRE(s < numeric_limits<size_t>::max());
  if (s == 0) {
    return 0;
  }

  ssize_t outSize_ = socket.recv(b, s);
  DA(outSize_ >= 0);
  auto outSize = static_cast<size_t>(outSize_);
  DA(outSize <= s);
  if (outSize == 0) {
    return numeric_limits<size_t>::max();
  }

  return outSize;
}

void TcpSocketStream::write (const iu8f *b, size_t s) {
  while (s != 0) {
    ssize_t outSize_ = socket.send(b, s);
    DA(outSize_ >= 0);
    auto outSize = static_cast<size_t>(outSize_);
    DA(outSize <= s);

    b += outSize;
    s -= outSize;
  }
}

void TcpSocketStream::close () {
  if (socket.closed()) {
    return;
  }

  DW(, "FINning writing side");
  try {
    // Send FIN for the writing side of the connection...
    socket.shutdown(SHUT_WR);

    // ... and then drain the reading side until it reaches FIN, so that we don't
    // induce an RST (see
    // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable).
    DW(, "draining reading side");
    iu8f dummy[BUFSIZ];
    size_t s;
    while ((s = read(dummy, sizeof(dummy) / sizeof(*dummy))) != numeric_limits<size_t>::max()) {
      DW(, "  read ", s, " bytes");
    }
    DW(, "reached reading stream EOF");
  } catch (...) {
    // We made our best effort. Oh, well.
    DW(, "hit an exception while doing graceful closedown");
  }

  DW(, "all other things done, now doing the final close() on the socket");
  socket.close();
}

PassiveTcpSocket::PassiveTcpSocket (const TcpSocketAddress &listenAddr, iu listenBacklog) : socket(listenAddr) {
  socket.bind(listenAddr);
  socket.listen(listenBacklog);
}

PassiveTcpSocket::PassiveTcpSocket (const TcpSocketAddress &listenAddr) : PassiveTcpSocket(listenAddr, SOMAXCONN) {
}

TcpSocketStream PassiveTcpSocket::accept (bool keepalive) {
  Socket s = socket.accept();
  s.setOptions(keepalive);
  return TcpSocketStream(move(s));
}

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}}
