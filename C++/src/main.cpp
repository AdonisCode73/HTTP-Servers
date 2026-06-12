#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <map>
#include <system_error>
#include <vector>
#include <memory>

#include "Socket.h"
#include "BufferedReader.h"

struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::vector<std::byte> body;
};


Socket getSocket() {

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* res = nullptr;
    if (int status = getaddrinfo(NULL, "8080", &hints, &res); status != 0) {
        throw std::runtime_error(std::string{"getaddrinfo: "} + gai_strerror(status));
    }
 
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> res_guard{res, freeaddrinfo};

    for (addrinfo* p = res; p != NULL; p = p->ai_next) {
        int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;

        Socket sock{fd};

        if (bind(sock.get_fd(), p->ai_addr, p->ai_addrlen) == 0) {
            return sock;
        }
    }

    throw std::runtime_error("could not bind to any address");
}

HTTPRequest parseHTTPRequest(Socket &connection) {
    BufferedReader reader(connection, 8192);

    std::cout << reader.readLine() << std::endl;

    return HTTPRequest{};
    /*return HTTPRequest {
        .method = "foo",
        .path = "foo",
        .version = "foo",
        .headers = {std::string("foo"), std::string("bar")},
        .body = {std::byte(13)}
    };*/
}

void handleConnection(Socket &connection) {
    std::cout << "Client successfully connected!" << std::endl;

    HTTPRequest req = parseHTTPRequest(connection);
} 

int main (int argc, char *argv[]) {
    std::cout << "Starting HTTP Server..." << std::endl;
    std::cout << "Waiting for client to connect." << std::endl;

    Socket sock = getSocket();

    if (listen(sock.get_fd(), 1) == -1) {
        throw std::system_error(errno, std::system_category(), "listen");
    }

    while (true) {
        try {
            struct sockaddr_storage theirAddr;
            socklen_t addrSize = sizeof(theirAddr);

            Socket connection(accept(sock.get_fd(), (struct sockaddr *)&theirAddr, &addrSize));

            if (connection.get_fd() < 0) {
                throw std::system_error(errno, std::system_category(), "accept");
            }

            handleConnection(connection);
        } catch (const std::system_error &e) {
            std::cerr << "connection error: " << e.what() << "(errno " << e.code() << ")" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
        }
    }

    return 0;
}
