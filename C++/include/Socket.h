#pragma once 

#include <unistd.h>
#include <utility>

class Socket {
public:
    explicit Socket(int fd) : m_fd(fd) {}
    ~Socket() {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
    }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : m_fd(std::exchange(other.m_fd, -1)) {};
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            if (m_fd >= 0) {
                ::close(m_fd);
                m_fd = std::exchange(other.m_fd, -1);
            }
        }
        return *this;
    }

    int get_fd() const { return m_fd; }


private:
    int m_fd = -1;
};
