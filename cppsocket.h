
#pragma once

#ifdef _WIN32

    #ifndef WINVER
        #define WINVER _WIN32_WINNT_WIN10
        #define _WIN32_WINNT _WIN32_WINNT_WIN10
    #endif

    #include <winsock2.h>
    #include <Ws2tcpip.h>

    // Link with ws2_32.lib
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/socket.h>
#endif

#include <cstddef>
#include <string>

namespace std
{
    typedef socklen_t sock_len;

#ifdef _WIN32
    typedef int addr_family;  // AF_*
#else
    typedef u_short addr_family;  // AF_*
#endif

    typedef int sock_type;  // SOCK_*
    typedef int sock_option;  // SO_*
    typedef int sock_level;  // SOL_*
    typedef int sock_shut;  // SHUT_*

    class sock_addr
    {
    protected:
        sock_addr(const sock_len& length = sizeof(addr_family));

    public:
        sock_addr(const sock_addr& x);

    public:
        addr_family family() const;
        sock_len length() const;
    
    protected:
        sockaddr& native();
        const sockaddr& native() const;
    
    protected:
#ifdef _WIN32
        SOCKADDR_STORAGE _native;
#else
        sockaddr_storage _native;
#endif
        sock_len _length;

    protected:
        friend class socket;
    };

    class sock_addr_inet final : public sock_addr
    {
    public:
#ifdef _WIN32
        typedef USHORT inet_port;
        typedef IN_ADDR inet_addr;
#else
#endif

    public:
        sock_addr_inet();
        sock_addr_inet(const std::string& addr, const inet_port& port);

        inet_port port() const;
        inet_addr addr() const;
        std::string addr_str() const;

    private:
        sockaddr_in& native_inet();
        const sockaddr_in& native_inet() const;
    };

    class socket final
    {
    private:
#ifdef _WIN32
        typedef SOCKET desc_type;
#else
        typedef int desc_type;
#endif

    public:
        explicit socket();
        explicit socket(addr_family domain, sock_type type);
        socket(const socket&) = delete;
        socket(socket&& x);

        ~socket();

        socket& operator=(const socket&) = delete;
        socket& operator=(socket&& x);

        operator bool() const;

        void accept(socket& sock);
        void accept(socket& sock, sock_addr& address);

        void bind(const sock_addr& address);
        void close();
        void connect(const sock_addr& address);
        void listen(const int& backlog = 0);
        void shutdown(const sock_shut& how);

#ifdef _WIN32
        void getsockopt(const sock_level& level, const sock_option& option_name,
                        char* option_value, sock_len& option_len) const;

        int recv(char* buffer, const int& length, const sock_option& flags = 0);
        template <typename T>
        int recv(T& buffer, const sock_option& flags = 0)
        {
            return recv((char*)&buffer, sizeof(T), flags);
        }

        int recvfrom(char* buffer, const int& length, const sock_option& flags, sock_addr& address);
        template <typename T>
        int recvfrom(T& buffer, const sock_option& flags, sock_addr& address)
        {
            return recvfrom((char*)&buffer, sizeof(T), flags, address);
        }

        int send(const char* buffer, const int& length, const sock_option& flags = 0);
        template <typename T>
        int send(T& buffer, const sock_option& flags = 0)
        {
            return send((char*)&buffer, sizeof(T), flags);
        }

        int sendto(const char* buffer, const int& length, const sock_option& flags, const sock_addr& dest_addr);
        template <typename T>
        int sendto(T& buffer, const sock_option& flags, const sock_addr& dest_addr)
        {
            return sendto((char*)&buffer, sizeof(T), flags, dest_addr);
        }

        void setsockopt(const sock_level& level, const sock_option& option_name,
                        const char* option_value, const sock_len& option_len);
#else
        void getsockopt(const sock_level& level, const sock_option& option_name,
                        void* option_value, sock_len& option_len) const;

        ssize_t recv(void* buffer, const size_t& length, const sock_option& flags = 0);
        ssize_t recv(T& buffer, const sock_option& flags = 0)
        {
            return recv(void*)&buffer, sizeof(T), flags);
        }

        ssize_t recvfrom(void* buffer, const size_t& length, const sock_option& flags, sock_addr& address);
        template <typename T>
        ssize_t recvfrom(T& buffer, const sock_option& flags, sock_addr& address)
        {
            return recvfrom(void*)&buffer, sizeof(T), flags, address);
        }

        ssize_t send(const void* buffer, const size_t& length, const sock_option& flags = 0);
        template <typename T>
        ssize_t send(T& buffer, const sock_option& flags = 0)
        {
            return send((void*)&buffer, sizeof(T), flags);
        }

        ssize_t sendto(const void *buffer, const size_t& length, const sock_option& flags, const sock_addr& dest_addr);
        template <typename T>
        ssize_t sendto(T& buffer, const sock_option& flags, const sock_addr& dest_addr)
        {
            return sendto((void*)&buffer, sizeof(T), flags, dest_addr);
        }

        void setsockopt(const sock_level& level, const sock_option& option_name,
                        const void* option_value, const sock_len& option_len);
#endif
    private:
        desc_type _sock;
    };
};
