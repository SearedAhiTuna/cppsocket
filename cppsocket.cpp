
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

    #define BAD_SOCKET -1
#endif

static void throw_error(const int& err)
{
    std::error_condition econd = std::system_category().default_error_condition(err);
    std::system_error(econd.value(), econd.category(), econd.message());
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
    sock_addr(sizeof(sockaddr_in))
{

}

std::sock_addr_inet::sock_addr_inet(const std::string& addr, const inet_port& port):
    sock_addr(sizeof(sockaddr_in))
{
    native_inet().sin_family = AF_INET;
    native_inet().sin_port = port;

#ifdef _WIN32
    inet_pton(AF_INET, addr.c_str(), &native_inet().sin_addr);
#else
    // Todo
#endif
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
    return std::string(inet_ntoa(addr()));
}

sockaddr_in& std::sock_addr_inet::native_inet()
{
    return reinterpret_cast<sockaddr_in&>(_native);
}

const sockaddr_in& std::sock_addr_inet::native_inet() const
{
    return reinterpret_cast<const sockaddr_in&>(_native);
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
    _sock = x._sock;
    x._sock = BAD_SOCKET;
    return *this;
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
    int err;
#ifdef _WIN32
    err = closesocket(_sock);
#else
    err = close(_sock);
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

#include <iostream>

int main(int argc, char** argv)
{
    if (std::string(argv[1]) == "client")
    {
        std::sock_addr_inet my_addr("127.0.0.1", 4400);
        std::cout << "Address = " << my_addr.addr_str() << ":" << my_addr.port() << "\n";
        std::cout.flush();

        std::socket sock(AF_INET, SOCK_STREAM);

        std::sock_addr_inet serv_addr("127.0.0.1", 4300);

        try
        {
            sock.connect(serv_addr);
            std::cout << "Connected to " << serv_addr.addr_str() << ":" << serv_addr.port() << "\n";

            std::string msg;
            do
            {
                std::cin >> msg;
                sock.send(msg.c_str(), msg.size());
            }
            while (msg != "done");
        }
        catch (std::exception& e)
        {
            std::cout << "Connection failed:" << e.what();
        }
    }
    else if (std::string(argv[1]) == "server")
    {
        std::sock_addr_inet my_addr("127.0.0.1", 4300);
        std::cout << "Address = " << my_addr.addr_str() << ":" << my_addr.port() << "\n";
        std::cout.flush();

        std::socket sock(AF_INET, SOCK_STREAM);

        try
        {
            sock.bind(my_addr);
            sock.listen();
        }
        catch (std::exception& e)
        {
            std::cout << "Bind and listen failed: " << e.what();
        }

        std::socket client_sock;
        std::sock_addr_inet client_addr;

        try
        {
            sock.accept(client_sock, client_addr);
            std::cout << "Connection from " << client_addr.addr_str() << ":" << client_addr.port() << "\n";
            std::cout.flush();

            char recv_buf[100];
            int recv_size;

            do
            {
                recv_size = client_sock.recv(recv_buf);

                if (recv_size > 0)
                {
                    std::cout << "Received: " << recv_buf << "\n";
                    std::cout.flush();
                }
            }
            while (recv_size > 0);
        }
        catch (std::exception e)
        {
            std::cout << "Accept failed: " << e.what();
        }
    }
    
    return 0;
}