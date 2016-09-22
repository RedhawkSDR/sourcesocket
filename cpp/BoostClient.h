/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file distributed with this 
 * source distribution.
 * 
 * This file is part of REDHAWK Basic Components sourcesocket.
 * 
 * REDHAWK Basic Components sourcesocket is free software: you can redistribute it and/or modify it under the terms of 
 * the GNU Lesser General Public License as published by the Free Software Foundation, either 
 * version 3 of the License, or (at your option) any later version.
 * 
 * REDHAWK Basic Components sourcesocket is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along with this 
 * program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef BOOSTCLIENT_H_
#define BOOSTCLIENT_H_

#include <iostream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <sstream>
#include <boost/asio/error.hpp>
#include <boost/shared_ptr.hpp>
#include <ossie/debug.h>

#include "BoundedBuffer.h"

using boost::asio::ip::tcp;

class Client {
ENABLE_LOGGING
public:
    Client(const unsigned short port, const std::string ip_addr,
            size_t buffer_len = 64, size_t max_sock_read_size = 64 * 1024);
    Client(const unsigned short port, const std::string ip_addr,
            size_t buffer_len, size_t max_sock_read_size, bool tcp_nodelay);
    ~Client();
    bool Connect(bool force = false);
    bool is_connected();
    bool is_empty();
    size_t Read(char* data, size_t size);

private:
    bool Start();
    void HandleRead(const boost::system::error_code& error,
            size_t bytes_transferred);
    void StartIoService();
    void RunIoService();
    void StopIoService();

    boost::asio::io_service io_service_;
    tcp::socket socket_;
    unsigned short port_;
    std::string ip_addr_;
    size_t max_sock_read_size_;
    std::vector<char> sock_read_buf_;
    BoundedBuffer<char> pending_buf_;
    boost::thread* thread_;
    bool tcp_nodelay_;
    bool configure_socket;
};

#endif /* BOOSTCLIENT_H_ */
