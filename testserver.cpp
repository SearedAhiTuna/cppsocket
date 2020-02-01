
#include "cppsocket.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Format:\n";
        std::cerr << "testserver <host ip> <port>\n";
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
        std::cerr << "testserver <host ip> <port>\n";
        return 1;
    }

    // Set up the sock and host address
    std::socket sock(AF_INET, SOCK_STREAM);
    std::sock_addr_inet host_addr(ip, port);

    try
    {
        std::cout << "Binding host to "
                  << host_addr.addr_str() << ":"
                  << host_addr.port() << "\n";
        std::cout.flush();

        // Bind to the address
        sock.bind(host_addr);

        // Listen for up to one connection at a time
        sock.listen(1);

        std::cout << "Listening for connections...\n";
        std::cout.flush();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // Client sock and address
    std::socket client_sock;
    std::sock_addr_inet client_addr;

    // Infinite loop
    for (;;)
    {
        try
        {
            // Accept a connection
            sock.accept(client_sock, client_addr);
            std::cout << "Accepted connection from "
                      << client_addr.addr_str() << ":"
                      << client_addr.port() << "\n";
            std::cout.flush();

            // Small receive buffer
            char recv_buf[100];
            int recv_size;

            // Loop until exception
            for (;;)
            {
                std::cout << "Listening...\n";
                std::cout.flush();

                // Receive data
                recv_size = client_sock.recv(recv_buf);

                // Add a null terminator to the data
                recv_buf[recv_size] = '\0';

                std::cout << "Received \"" << recv_buf << "\".\n";
            }
        }
        catch (const std::exception& e)
        {
            // Report the exception and go back to listening
            std::cerr << e.what() << "\n";
        }
    }

    return 0;
}