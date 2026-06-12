#include <asm-generic/socket.h>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <unordered_map>
#include <system_error>
#include <memory>
#include <format>
#include <algorithm>

#include "Socket.h"
#include "BufferedReader.h"

enum class StatusCode {
    Ok,
    BadRequest,
    NotFound,
    MethodNotAllowed,
    InternalServerError
};

struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HTTPResponse {
    StatusCode statusCode;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};



HTTPResponse homeHandler(const HTTPRequest&);
HTTPResponse helloHandler(const HTTPRequest&);
HTTPResponse invalidPathHandler(const HTTPRequest&);
HTTPResponse createUser(const HTTPRequest&);

using Handler = HTTPResponse (*)(const HTTPRequest&);

const std::unordered_map<std::string, std::unordered_map<std::string, Handler>> MethodHandlers {{
        "GET", {
            {   "/", homeHandler    },
            {   "/hello", helloHandler  }
        }},
    {
        "POST", {
                {   "/users", createUser    }
        }},
};

const std::unordered_map<StatusCode, std::string_view> StatusLineResponses {
    
    {   StatusCode::Ok,                     "HTTP/1.1 200 OK\r\n"   },
    {   StatusCode::BadRequest,             "HTTP/1.1 400 Bad Request\r\n"  },
    {   StatusCode::NotFound,               "HTTP/1.1 404 Not Found\r\n"  },
    {   StatusCode::MethodNotAllowed,       "HTTP/1.1 405 Method Not Allowed\r\n" },
    {   StatusCode::InternalServerError,   "HTTP/1.1 500 Internal Server Error\r\n"  }
};

HTTPResponse homeHandler(const HTTPRequest& _) {
    return HTTPResponse {
        .statusCode = StatusCode::Ok,
        .headers = std::unordered_map<std::string, std::string>{},
        .body = "HTTP server in C++ exercise, nothing exciting here"
    };
}

HTTPResponse helloHandler(const HTTPRequest& _) {
    return HTTPResponse {
        .statusCode = StatusCode::Ok,
        .headers = std::unordered_map<std::string, std::string>{},
        .body = "Hello to you too!"
    };
}

HTTPResponse invalidPathHandler(const HTTPRequest& _) {
    return HTTPResponse {
        .statusCode = StatusCode::Ok,
        .headers = std::unordered_map<std::string, std::string>{},
        .body = "Invald path provided"
    };
}

HTTPResponse createUser(const HTTPRequest& req) {
    std::string_view body = req.body;

    std::size_t colon = body.find(":");
    if (colon == std::string::npos) {
        throw std::runtime_error("Invalid syntax on POST create user body");
    }

    std::string_view value = body.substr(colon + 1);

    auto isJunk = [](char c) {
        return c == ' ' || c == '"' || c == '{' || c == '}';
    };

    while (!value.empty() && isJunk(value.front()))  { value.remove_prefix(1);  }
    while (!value.empty() && isJunk(value.back()))   { value.remove_suffix(1);  } 

    auto response = std::format("User {} created successfully", value);

    return HTTPResponse {
        .statusCode = StatusCode::Ok,
        .headers = std::unordered_map<std::string, std::string>{},
        .body = response 
    };
}

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

       int yes = 1;

        if (setsockopt(sock.get_fd(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
            throw std::system_error(errno, std::system_category(), "setsockopt(REUSEADDR)");
        }

        if (bind(sock.get_fd(), p->ai_addr, p->ai_addrlen) == 0) {
            return sock;
        }
    }

    throw std::runtime_error("could not bind to any address");
}

HTTPRequest parseHTTPRequest(Socket &connection) {
    BufferedReader reader(connection, 8192);

    std::string line = reader.readLine();
    std::istringstream requestLine(line);
    std::string method, path, version;

    requestLine >> method >> path >> version;

    version = version.substr(0, '\n');
    std::unordered_map<std::string, std::string> headers{};
    std::string body;

    while (true) {
        line.clear();
        line = reader.readLine();

        if (line == "\r\n") {
            break;
        }

        const auto noSpaceEnd = std::remove(line.begin(), line.end(), ' ');
        line.erase(noSpaceEnd, line.end());

        auto colon = line.find(":");
        if (colon == std::string::npos) {
            throw std::runtime_error(std::string{"Failure parsing header, no colon :"});
        }
        headers[line.substr(0, colon)] = line.substr(colon + 1);
    }

    if (headers.contains("Content-Length")) {
        std::string& lengthString = headers["Content-Length"];
        size_t length = 0;

        auto [ptr, ec] = std::from_chars(lengthString.data(), lengthString.data() + lengthString.size(), length);

        if (ec != std::errc{}){
            throw std::runtime_error("Invalid Content-Length");
        }
       
        std::cout << length << std::endl;
        body = reader.read_exact(length);
    }

    return HTTPRequest {
        .method = method,
        .path = path,
        .version = version,
        .headers = headers,
        .body = body
    };
}

void writeHTTPResponse(const HTTPResponse& resp, Socket& connection, bool returnsContent) {
   
    std::string_view statusLine = StatusLineResponses.at(resp.statusCode);
    send(connection.get_fd(), statusLine.data(), statusLine.size(), 0);

    for (auto& [name, value] : resp.headers) {
        std::string msg = name + ": " + value + "\r\n";
        send(connection.get_fd(), msg.data(), msg.size(), 0);
    }

    std::string end = "\r\n";
    send(connection.get_fd(), end.data(), end.size(), 0);

    if (returnsContent) {
        send(connection.get_fd(), resp.body.data(), resp.body.size(), 0);
    }
}

void handleConnection(Socket &connection) {
    std::cout << "Client successfully connected!" << std::endl;

    HTTPRequest req = parseHTTPRequest(connection);

    Handler hf = MethodHandlers.at(req.method).at(req.path);
    if (hf == nullptr) {
        hf = invalidPathHandler;
    }
    HTTPResponse resp = hf(req);

    bool returnsContent = false;

    if (resp.body.size() > 0) {
        returnsContent = true;
        resp.headers["Content-Length"] = std::to_string(resp.body.size());
    }

    writeHTTPResponse(resp, connection, returnsContent);
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
