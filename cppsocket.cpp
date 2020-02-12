
#include "cppsocket.h"

#include <cstring>
#include <system_error>

#ifdef _WIN32
    #define BAD_SOCKET INVALID_SOCKET

    static bool wsa_init = false;
    static WSADATA wsa_data = {0};
    static unsigned long wsa_count = 0;
#else
    #include <errno.h>
    #include <arpa/inet.h>
    #include <unistd.h>

    #define BAD_SOCKET -1
#endif

static void throw_error(const int& err)
{
#ifdef _WIN32
    // Get the error message
    LPSTR msg_buf = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    size_t size = FormatMessageA(flags, NULL, err, lang_id, (LPSTR)&msg_buf, 0, NULL);

    // Copy to string
    std::string message(msg_buf, size);

    // Free the old buffer
    LocalFree(msg_buf);

    throw std::system_error(err, std::system_category(), message);
#else
    std::error_condition econd = std::system_category().default_error_condition(err);
    throw std::system_error(econd.value(), econd.category(), econd.message());
#endif
}
static void throw_error()
{
#ifdef _WIN32
    throw_error(WSAGetLastError());
#else
    throw_error(errno);
#endif
}

std::sock_addr::sock_addr(const sock_len& length):
    _length(length)
{
}

std::sock_addr::sock_addr(const sock_addr& x):
    _length(x._length),
    _native(x._native)
{
}

std::addr_family std::sock_addr::family() const
{
    return native().sa_family;
}

std::sock_len std::sock_addr::length() const
{
    return _length;
}

sockaddr& std::sock_addr::native()
{
    return reinterpret_cast<sockaddr&>(_native);
}
const sockaddr& std::sock_addr::native() const
{
    return reinterpret_cast<const sockaddr&>(_native);
}

std::sock_addr_inet::sock_addr_inet():
    sock_addr(sizeof(inet_native))
{
}

std::sock_addr_inet::sock_addr_inet(const std::string& addr, const inet_port& port):
    sock_addr(sizeof(inet_native))
{
    std::string actualAddr;
    if (addr == "localhost")
        actualAddr = "127.0.0.1";
    else
        actualAddr = addr;

    native_inet().sin_family = AF_INET;
    native_inet().sin_port = port;
    inet_pton(AF_INET, actualAddr.c_str(), &native_inet().sin_addr);
}

std::sock_addr_inet::inet_port std::sock_addr_inet::port() const
{
    return native_inet().sin_port;
}

std::sock_addr_inet::inet_addr std::sock_addr_inet::addr() const
{
    return native_inet().sin_addr;
}

std::string std::sock_addr_inet::addr_str() const
{
#ifdef _WIN32
    char buf[16];
    return std::string(inet_ntop(AF_INET, &addr(), buf, 16));
#else
    return std::string(inet_ntoa(addr()));
#endif
}

std::sock_addr_inet::inet_native& std::sock_addr_inet::native_inet()
{
    return reinterpret_cast<inet_native&>(_native);
}

const std::sock_addr_inet::inet_native& std::sock_addr_inet::native_inet() const
{
    return reinterpret_cast<const inet_native&>(_native);
}

std::socket::socket():
    _sock(BAD_SOCKET)
{
}

std::socket::socket(addr_family domain, sock_type type)
{
#ifdef _WIN32
    if (!wsa_init)
    {
        wsa_init = WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
    }
#endif

    _sock = ::socket(domain, type, 0);

#ifdef _WIN32
    if (_sock != BAD_SOCKET)
    {
        ++wsa_count;
    }
#endif

    if (_sock == BAD_SOCKET)
    {
        throw_error();
    }
}

std::socket::socket(socket&& x)
{
    _sock = x._sock;
    x._sock = BAD_SOCKET;
}

std::socket::~socket()
{
    if (*this)
    {
        close();

#ifdef _WIN32
        if (--wsa_count == 0)
        {
            WSACleanup();
        }
#endif
    }
}

std::socket& std::socket::operator=(socket&& x)
{
    if (*this)
    {
        close();
    }

    _sock = x._sock;
    x._sock = BAD_SOCKET;
    return *this;
}

std::socket::operator std::native_socket_desc() const
{
    return _sock;
}

std::socket::operator bool() const
{
    return _sock != BAD_SOCKET;
}

void std::socket::accept(socket& sock)
{
    sock_addr dummy;
    accept(sock, dummy);
}
void std::socket::accept(socket& sock, sock_addr& address)
{
    if (sock)  // Close the input socket if it is open
    {
        sock.close();
    }

    sock._sock = ::accept(_sock, &address.native(), &address._length);
    if (sock._sock == BAD_SOCKET)
    {
        throw_error();
    }
}
void std::socket::bind(const sock_addr& address)
{
    int err = ::bind(_sock, &address.native(), address.length());
    if (err == BAD_SOCKET)
    {
        throw_error();
    }
}

void std::socket::close()
{
    if (!(*this))
        return;

    int err;
#ifdef _WIN32
    err = closesocket(_sock);
#else
    err = ::close(_sock);
#endif

    if (err == BAD_SOCKET)
    {
        throw_error();
    }

    if (err != BAD_SOCKET)
    {
        _sock = BAD_SOCKET;

#ifdef _WIN32
        if (--wsa_count == 0)
        {
            WSACleanup();
            wsa_init = false;
        }
#endif
    }
}

void std::socket::connect(const std::sock_addr& address)
{
    int err = ::connect(_sock, &address.native(), address.length());
    if (err == BAD_SOCKET)
    {
        throw_error();
    }
}

void std::socket::listen(const int& backlog)
{
    int err = ::listen(_sock, backlog);
    if (err == BAD_SOCKET)
    {
        throw_error();
    }
}

void std::socket::shutdown(const std::sock_shut& how)
{
    int err = ::shutdown(_sock, how);
    if (err == BAD_SOCKET)
    {
        throw_error();
    }
}

#ifdef _WIN32
void std::socket::getsockopt(const sock_level& level, const sock_option& option_name,
                             char* option_value, sock_len& option_len) const
#else
void std::socket::getsockopt(const sock_level& level, const sock_option& option_name,
                             void* option_value, sock_len& option_len) const
#endif
{
    int err = ::getsockopt(_sock, level, option_name, option_value, &option_len);
}

#ifdef _WIN32
int std::socket::recv(char* buffer, const int& length, const sock_option& flags)
#else
ssize_t std::socket::recv(void* buffer, const size_t& length, const sock_option& flags)
#endif
{
#ifdef _WIN32
    int size;
#else
    ssize_t size;
#endif

    size = ::recv(_sock, buffer, length, flags);

    if (size == BAD_SOCKET)
    {
        throw_error();
    }

    return size;
}

#ifdef _WIN32
int std::socket::recvfrom(char* buffer, const int& length, const sock_option& flags, sock_addr& address)
#else
ssize_t std::socket::recvfrom(void* buffer, const size_t& length, const sock_option& flags, sock_addr& address)
#endif
{
#ifdef _WIN32
    int size;
#else
    ssize_t size;
#endif

    size = ::recvfrom(_sock, buffer, length, flags, &address.native(), &address._length);

    if (size == BAD_SOCKET)
    {
        throw_error();
    }

    return size;
}

#ifdef _WIN32
int std::socket::send(const char* buffer, const int& length, const sock_option& flags)
#else
ssize_t std::socket::send(const void* buffer, const size_t& length, const sock_option& flags)
#endif
{
#ifdef _WIN32
    int size;
#else
    ssize_t size;
#endif

    size = ::send(_sock, buffer, length, flags);

    if (size == BAD_SOCKET)
    {
        throw_error();
    }

    return size;
}

#ifdef _WIN32
int std::socket::sendto(const char* buffer, const int& length, const sock_option& flags, const sock_addr& dest_addr)
#else
ssize_t std::socket::sendto(const void *buffer, const size_t& length, const sock_option& flags, const sock_addr& dest_addr)
#endif
{
#ifdef _WIN32
    int size;
#else
    ssize_t size;
#endif

    size = ::sendto(_sock, buffer, length, flags, &dest_addr.native(), dest_addr.length());

    if (size == BAD_SOCKET)
    {
        throw_error();
    }

    return size;
}

#ifdef _WIN32
void std::socket::setsockopt(const sock_level& level, const sock_option& option_name,
                             const char* option_value, const sock_len& option_len)
#else
void std::socket::setsockopt(const sock_level& level, const sock_option& option_name,
                             const void* option_value, const sock_len& option_len)
#endif
{
    int err = ::setsockopt(_sock, level, option_name, option_value, option_len);
    if (err == BAD_SOCKET)
    {
        throw_error();
    }
}
