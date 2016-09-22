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

#include "BoostClient.h"

PREPARE_LOGGING(Client)

Client::Client(const unsigned short port, const std::string ip_addr,
        size_t buffer_len, size_t max_sock_read_size) :
        socket_(io_service_), port_(port), ip_addr_(ip_addr), max_sock_read_size_(
                max_sock_read_size), sock_read_buf_(max_sock_read_size), pending_buf_(
                buffer_len * max_sock_read_size), thread_(NULL), tcp_nodelay_(
                false), configure_socket(false) {
    LOG_DEBUG(Client, __PRETTY_FUNCTION__ << " max_sock_read_size_=" << max_sock_read_size_
                                          << " sock_read_buf_.size()=" << sock_read_buf_.size()
                                          << " pending_buf_.size()=" << pending_buf_.size()
                                          << " pending_buf_.capacity()=" << pending_buf_.capacity());
}

Client::Client(const unsigned short port, const std::string ip_addr,
        size_t buffer_len, size_t max_sock_read_size, bool tcp_nodelay) :
        socket_(io_service_), port_(port), ip_addr_(ip_addr), max_sock_read_size_(
                max_sock_read_size), sock_read_buf_(max_sock_read_size), pending_buf_(
                buffer_len * max_sock_read_size), thread_(NULL), tcp_nodelay_(
                tcp_nodelay), configure_socket(true) {
    LOG_DEBUG(Client, __PRETTY_FUNCTION__ << " max_sock_read_size_=" << max_sock_read_size_
                                          << " sock_read_buf_.size()=" << sock_read_buf_.size()
                                          << " pending_buf_.size()=" << pending_buf_.size()
                                          << " pending_buf_.capacity()=" << pending_buf_.capacity());
}

Client::~Client() {
    if (thread_) {
        io_service_.stop();
        thread_->join();
        delete thread_;
    }
}

bool Client::Connect(bool force) {
    LOG_DEBUG(Client, this->port_<< " Client Connect entry");
    LOG_DEBUG(Client, __PRETTY_FUNCTION__ << " max_sock_read_size_=" << max_sock_read_size_
                                          << " sock_read_buf_.size()=" << sock_read_buf_.size()
                                          << " pending_buf_.size()=" << pending_buf_.size()
                                          << " pending_buf_.capacity()=" << pending_buf_.capacity());
    if (is_connected() && !force)
        return true;
    try {
        tcp::resolver resolver(io_service_);
        std::stringstream ss;
        ss << port_;
        tcp::resolver::query query(ip_addr_, ss.str());
        tcp::resolver::iterator iter = resolver.resolve(query);
        socket_.connect(*iter);
        bool connected = Start();
        if (connected) {
            LOG_INFO(Client,
                    this->port_<< " Client connected " << ip_addr_ << ":" << port_);
            return true;
        }
        LOG_DEBUG(Client,
                this->port_<< " Client Connect - not connected, return");
        return false;
    } catch (std::exception& e) {
        LOG_DEBUG(Client,
                this->port_<< " Client Connect - exception..." << e.what());
        socket_.close();
        return false;
    }
}

bool Client::is_connected() {
    return socket_.is_open();
}

bool Client::is_empty() {
    return pending_buf_.empty();
}

bool Client::Start() {
    LOG_DEBUG(Client, this->port_<< " Client Start entry");
    if (is_connected()) {
        boost::asio::ip::tcp::no_delay option1;
        boost::asio::socket_base::receive_buffer_size option2;

        socket_.get_option(option1);
        socket_.get_option(option2);
        bool actual_tcpnodelay = option1.value();
        int actual_rx_buff_sz = option2.value();
        LOG_DEBUG(Client,"BEFORE: receive_buffer_size is "<<actual_rx_buff_sz<<" and tcp nodelay is "<<actual_tcpnodelay);

        if( configure_socket ) {
            //boost::asio::ip::tcp::no_delay option1(tcp_nodelay_);
            //boost::asio::socket_base::receive_buffer_size option2(max_sock_read_size_);
            option1 = tcp_nodelay_;
            option2 = max_sock_read_size_;
            socket_.set_option(option1);
            socket_.set_option(option2);
        }

        socket_.get_option(option1);
        socket_.get_option(option2);
        actual_tcpnodelay = option1.value();
        actual_rx_buff_sz = option2.value();
        LOG_INFO(Client,"receive_buffer_size is "<<actual_rx_buff_sz<<" and tcp nodelay is "<<actual_tcpnodelay);

        // Read the response status line.
        socket_.async_read_some(boost::asio::buffer(sock_read_buf_),
                boost::bind(&Client::HandleRead, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        StartIoService();
        return true;
    }
    return false;
    LOG_DEBUG(Client, this->port_<< " Client Start exit");
}

size_t Client::Read(char* data, size_t size) {
    LOG_DEBUG(Client, this->port_<< " Client Read entry");
    return pending_buf_.tryread(data, size);
}

void Client::HandleRead(const boost::system::error_code& error,
        size_t bytes_transferred) {
    LOG_DEBUG(Client, this->port_<< __PRETTY_FUNCTION__ << " entry: bytes_transferred=" << bytes_transferred);
    LOG_DEBUG(Client, __PRETTY_FUNCTION__ << " enter: max_sock_read_size_=" << max_sock_read_size_
                                          << " sock_read_buf_.size()=" << sock_read_buf_.size()
                                          << " pending_buf_.size()=" << pending_buf_.size()
                                          << " pending_buf_.capacity()=" << pending_buf_.capacity());
    if (!error) {
        LOG_DEBUG(Client, this->port_<< __PRETTY_FUNCTION__ << " no error");
        pending_buf_.write(&sock_read_buf_[0], bytes_transferred);
        socket_.async_read_some(boost::asio::buffer(sock_read_buf_),
                boost::bind(&Client::HandleRead, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    } else {
        if (error == boost::asio::error::eof) {
            LOG_INFO(Client,
                    this->port_<< " Client closed connection, closing session: "<<error);
        } else {
            LOG_ERROR(Client,
                    this->port_<< " Error reading session data, closing session: "<<error);
        }
        socket_.close();
    }
    LOG_DEBUG(Client, __PRETTY_FUNCTION__ << " exit: max_sock_read_size_=" << max_sock_read_size_
                                          << " sock_read_buf_.size()=" << sock_read_buf_.size()
                                          << " pending_buf_.size()=" << pending_buf_.size()
                                          << " pending_buf_.capacity()=" << pending_buf_.capacity());
    LOG_DEBUG(Client, this->port_<< " Client HandleRead exit");
}

void Client::StartIoService() {
    LOG_DEBUG(Client, this->port_<< " Client StartIoService entry");
    if (thread_) {
        StopIoService();
    }
    thread_ = new boost::thread(boost::bind(&Client::RunIoService, this));
    LOG_DEBUG(Client, this->port_<< " Client StartIoService exit");
}

void Client::RunIoService() {
    LOG_DEBUG(Client, this->port_<< " Client RunIoService entry");
    try {
        io_service_.run();
    } catch (std::exception& e) {
        std::cerr << "Exception in io_service.run: " << e.what() << "\n";
        std::exit(1);
    }
    try {
        io_service_.reset();
    } catch (std::exception& e) {
        std::cerr << "Exception in io_service.reset: " << e.what() << "\n";
        std::exit(1);
    }/*
     try {
         if (socket_.is_open()) {
         LOG_DEBUG(Client,
             this->port_<< " Client RunIoService - socket is open, closing socket");
         socket_.close();
         }
     } catch (std::exception& e) {
         std::cerr << "Exception in socket.close: " << e.what() << "\n";
         std::exit(1);
     }*/
    LOG_DEBUG(Client, this->port_<< " Client RunIoService exit");
}

void Client::StopIoService() {
    LOG_DEBUG(Client, this->port_<< " Client StopIoService entry");
    if (thread_) {
        io_service_.stop();
        io_service_.reset();
        thread_->join();
        delete thread_;
    }
    LOG_DEBUG(Client, this->port_<< " Client StopIoService exit");
}
