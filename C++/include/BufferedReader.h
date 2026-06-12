#pragma once 

#include <cstddef>
#include <vector>
#include <string>
#include "Socket.h"

class BufferedReader {
    public:
        explicit BufferedReader(Socket& s, size_t bufferLength) : m_sock(s), m_buffer(bufferLength) {}

        std::string readLine();
        std::vector<std::byte> read_exact(size_t n);

    private:
        Socket& m_sock;
        std::vector<char> m_buffer;

        size_t m_readPos = 0;
        size_t m_writePos = 0;

        void refill();
};
