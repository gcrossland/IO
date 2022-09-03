/** @file */
/* -----------------------------------------------------------------------------
   Socket I/O Library
   © Geoff Crossland 2017-2022
----------------------------------------------------------------------------- */
#ifndef IO_SOCKET_ALREADYINCLUDED
#define IO_SOCKET_ALREADYINCLUDED

#include <core.hpp>
#include <iterators.hpp>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <vector>

namespace io::socket {

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
extern DC();

/**
  Represents an IP address plus a TCP port.
*/
class TcpSocketAddress {
  /**
    The corresponding socket type ({@c SOCK_STREAM}).
  */
  pub static constexpr int type = SOCK_STREAM;
  /**
    The corresponding protocol ({@c IPPROTO_TCP}).
  */
  pub static constexpr int protocol = IPPROTO_TCP;

  prv union {
    sockaddr_in socketAddr4;
    sockaddr_in6 socketAddr6;
  } _;

  prv explicit TcpSocketAddress (const sockaddr_in &socketAddr4);
  prv explicit TcpSocketAddress (const sockaddr_in6 &socketAddr6);

  /**
    Gets the corresponding family (one of {@c AF_INET} or {@c AF_INET6}).
  */
  pub sa_family_t getFamily () const noexcept;
  /**
    Returns a pointer to and the {@c sizeof} a {@c sockaddr} structure that
    describes this object.
  */
  pub std::tuple<const sockaddr *, socklen_t> getSocketAddress () const noexcept;
  /**
    Gets a human-readable representation of this object.
  */
  pub void getSocketAddress (core::u8string &r_r) const noexcept;

  prv static void get (std::vector<TcpSocketAddress> &r_addrs, const char8_t *nodeName, iu16f port);
  /**
    Resolves (via {@c getaddrinfo}) the given node name (be it a network hostname
    or a numerical network address string) and gets a sequence of TcpSocketAddress
    objects that each identify the given port on that node.
  */
  pub static void get (std::vector<TcpSocketAddress> &r_addrs, const core::u8string &nodeName, iu16f port);
  /**
    Resolves (via {@c getaddrinfo}) the given node name (be it a network hostname
    or a numerical network address string) and gets a sequence of TcpSocketAddress
    objects that each identify the given port on that node. The node name and port
    are supplied in a single string, where the node name is followed by a colon
    and then by the port number in decimal.

    @throw if the node name and port string is not of the required format.
  */
  pub static void get (std::vector<TcpSocketAddress> &r_addrs, const core::u8string &nodeNameAndPort);
  /**
    Gets (via {@c getaddrinfo}) a sequence of TcpSocketAddress objects that each
    identify the given port on any of this host's network addresses (with the
    wildcard address).
  */
  pub static void get (std::vector<TcpSocketAddress> &r_addrs, iu16f port);
};

class Socket {
  prv int s;

  prv explicit Socket (decltype(s) s);
  prv Socket (sa_family_t family, int type, int protocol);
  pub explicit Socket (const TcpSocketAddress &addr);
  Socket (const Socket &) = delete;
  Socket &operator= (const Socket &) = delete;
  pub Socket (Socket &&) noexcept;
  pub Socket &operator= (Socket &&) noexcept;
  pub ~Socket () noexcept;

  pub void bind (const TcpSocketAddress &addr);
  pub void listen (iu listenBacklog);
  pub Socket accept ();
  pub void connect (const TcpSocketAddress &addr);
  pub void setOptions (bool keepalive);
  pub ssize_t recv (void *buf, size_t len);
  pub ssize_t send (const void *buf, size_t len);
  pub void shutdown (int how);
  pub void close ();
  pub bool closed () const noexcept;
};

/**
  An {@c InputStream} and {@c OutputStream} for TCP sockets.
*/
class TcpSocketStream {
  prv Socket socket;

  prv explicit TcpSocketStream (Socket &&socket);
  /**
    Creates a socket and connects it to the given address.
   */
  pub TcpSocketStream (const TcpSocketAddress &targetAddr, bool keepalive);

  pub size_t read (iu8f *b, size_t s);
  pub void write (const iu8f *b, size_t s);
  pub void close ();

  friend class PassiveTcpSocket;
};

/**
  Manages a passive TCP socket.
*/
class PassiveTcpSocket {
  pub Socket socket;

  /**
    Creates a socket and configures it for listening (with the given maximum
    outstanding connection backlog size) at the given address.
  */
  pub PassiveTcpSocket (const TcpSocketAddress &listenAddr, iu listenBacklog);
  /**
    Creates a socket and configures it for listening (with a default maximum
    outstanding connection backlog size) at the given address.
  */
  pub explicit PassiveTcpSocket (const TcpSocketAddress &listenAddr);

  /**
    Waits for connections to the address associated with this socket and returns
    a TcpSocketStream for the first.
  */
  pub TcpSocketStream accept (bool keepalive);
};

/* -----------------------------------------------------------------------------
----------------------------------------------------------------------------- */
}

#endif
