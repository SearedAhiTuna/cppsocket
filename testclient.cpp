
#include "cppsocket.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Format:\n";
        std::cerr << "testclient <host ip> <port>\n";
        return 1;
    }

    // Get the IP address
    std::string ip(argv[1]);
    if (ip == "localhost")
        ip = "127.0.0.1";

    // Get the port
    int port;
    try
    {
        port = std::stoi(argv[2]);
    }
    catch(...)
    {
        std::cerr << "Invalid port.\n";
        std::cerr << "Format:\n";
        std::cerr << "testclient <host ip> <port>\n";
        return 1;
    }

    // Set up the sock and host address
    std::socket sock(AF_INET, SOCK_STREAM);
    std::sock_addr_inet host_addr(ip, port);

    try
    {
        std::cout << "Connecting to host at "
                      << host_addr.addr_str() << ":"
                      << host_addr.port() << "...\n";
        std::cout.flush();

        // Connect to the host
        sock.connect(host_addr);
        std::cout << "Connected.\n";

        // Send buffer
        std::string buf;

        // Infinite loop
        for (;;)
        {
            std::cout << ">";
            std::getline(std::cin, buf);

            std::cout << "Sending \"" << buf << "\"...\n";
            std::cout.flush();
            sock.send(buf.c_str(), buf.size());

            std::cout << "Sent.\n";
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}