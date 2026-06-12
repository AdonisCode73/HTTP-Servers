#include "BufferedReader.h"
#include <cstddef>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <vector>
#include <sys/socket.h>

std::string BufferedReader::readLine(){
    std::string line;

    while (true) {
       if (m_readPos == m_writePos) {
            refill();
            m_readPos = 0;

            if (m_writePos == 0) {
                break; // EOF
            } 
        }

        char c = m_buffer[m_readPos];
        line.append(1, c);

        m_readPos++;

        if (c == '\n') {
            break;
        }
    }

    return line;
}

std::vector<std::byte> BufferedReader::read_exact(size_t n) {

}

void BufferedReader::refill() {
    ssize_t n = recv(m_sock.get_fd(), m_buffer.data(), m_buffer.size(), 0);

    if (n < 0) {
        throw std::system_error(errno, std::system_category(), "recv");
    }

    m_writePos = static_cast<size_t>(n);
}
