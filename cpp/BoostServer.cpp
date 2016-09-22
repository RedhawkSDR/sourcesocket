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

#include "BoostServer.h"

PREPARE_LOGGING(Session)
PREPARE_LOGGING(Server)

Session::Session(boost::asio::io_service& io_service, Server* s,
        size_t max_sock_read_size, short port) :
        socket_(io_service), server_(s), sock_read_buf_(max_sock_read_size) {
    port_ = port;
}

tcp::socket& Session::Socket() {
    return socket_;
}

void Session::Start() {
    //boost::asio::socket_base::receive_buffer_size option;
    //socket_.get_option(option);
    //LOG_INFO(Session,this->port_<<"::Session::Start - receive_buffer_size is "<<option.value());
    socket_.async_read_some(boost::asio::buffer(sock_read_buf_),
            boost::bind(&Session::HandleRead, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
}

void Session::HandleRead(const boost::system::error_code& error,
        size_t bytes_transferred) {
    if (!error) {
        LOG_DEBUG(Session,
                this->port_<< " Read " << bytes_transferred << " from socket; sending data to server");
        server_->NewSessionData((char*) &sock_read_buf_[0], bytes_transferred);
        socket_.async_read_some(boost::asio::buffer(sock_read_buf_),
                boost::bind(&Session::HandleRead, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    } else {
        if (error == boost::asio::error::eof) {
            LOG_DEBUG(Session,
                    this->port_<< " Client closed connection, closing session: "<<error);
        } else {
            LOG_ERROR(Session,
                    this->port_<< " Error reading session data, closing session: "<<error);
        }
        server_->CloseSession(shared_from_this());
    }
}

Server::Server(short port, size_t buffer_len, size_t max_sock_read_size) :
        acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), thread_(NULL), max_sock_read_size_(
                max_sock_read_size), pending_buf_(buffer_len * max_sock_read_size), port_(
                port), tcp_nodelay_(false), configure_socket(false) {
    StartAccept();
    thread_ = new boost::thread(boost::bind(&Server::RunIoService, this));
}

Server::Server(short port, size_t buffer_len, size_t max_sock_read_size, bool tcp_nodelay) :
        acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), thread_(NULL), max_sock_read_size_(
                max_sock_read_size), pending_buf_(buffer_len * max_sock_read_size), port_(
                port), tcp_nodelay_(tcp_nodelay), configure_socket(true) {
    StartAccept();
    thread_ = new boost::thread(boost::bind(&Server::RunIoService, this));
}

Server::~Server() {
    {
        boost::mutex::scoped_lock lock(sessions_lock_);
        sessions_.clear();
    }
    if (thread_) {
        io_service_.stop();
        thread_->join();
        delete thread_;
    }
}

size_t Server::Read(char* data, size_t size) {
    LOG_DEBUG(Server,
            this->port_<< "::Server::read() requesting "<<size<<" Bytes from pending_buf_, size="<<pending_buf_.size());
    size_t numBytes = pending_buf_.tryread(data, size);
    LOG_DEBUG(Server,
            this->port_<< "::Server::read() got "<<numBytes<<" Bytes from pending_buf_, size="<<pending_buf_.size());
    return numBytes;
}

bool Server::is_connected() {
    return !sessions_.empty();
}

bool Server::is_empty() {
    return pending_buf_.empty();
}

void Server::NewSessionData(char* data, size_t size) {
    LOG_DEBUG(Server,
            this->port_<< " newSessionData attempting to write "<<size<<" bytes to pending_buf_, size="<<pending_buf_.size());
    size_t numBytes = pending_buf_.write(data, size);
    LOG_DEBUG(Server,
            this->port_<< " newSessionData wrote "<<numBytes<<" to pending_buf_, size="<<pending_buf_.size());

}

void Server::CloseSession(SessionPtr ptr) {
    boost::mutex::scoped_lock lock(sessions_lock_);
    for (std::list<SessionPtr>::iterator i = sessions_.begin();
            i != sessions_.end(); i++) {
        if (ptr == *i) {
            sessions_.remove(ptr);
            break;
        }
    }
}

void Server::StartAccept() {
    {
        SessionPtr new_session(
                new Session(io_service_, this, max_sock_read_size_,
                        this->port_));
        acceptor_.async_accept(new_session->Socket(),
                boost::bind(&Server::HandleAccept, this, new_session,
                        boost::asio::placeholders::error));
    }
}

void Server::HandleAccept(SessionPtr new_session,
        const boost::system::error_code& error) {
    if (!error) {
        {
            boost::asio::ip::tcp::no_delay option1;
            boost::asio::socket_base::receive_buffer_size option2;

            new_session->Socket().get_option(option1);
            new_session->Socket().get_option(option2);
            bool actual_tcpnodelay = option1.value();
            int actual_rx_buff_sz = option2.value();
            LOG_DEBUG(Server,"BEFORE: receive_buffer_size is "<<actual_rx_buff_sz<<" and tcp nodelay is "<<actual_tcpnodelay);

            if( configure_socket ) {
                //boost::asio::ip::tcp::no_delay option1(tcp_nodelay_);
                //boost::asio::socket_base::receive_buffer_size option2(max_sock_read_size_);
                option1 = tcp_nodelay_;
                option2 = max_sock_read_size_;
                new_session->Socket().set_option(option1);
                new_session->Socket().set_option(option2);
            }

            new_session->Socket().get_option(option1);
            new_session->Socket().get_option(option2);
            actual_tcpnodelay = option1.value();
            actual_rx_buff_sz = option2.value();
            LOG_INFO(Server,"receive_buffer_size is "<<actual_rx_buff_sz<<" and tcp nodelay is "<<actual_tcpnodelay);

            boost::mutex::scoped_lock lock(sessions_lock_);
            sessions_.push_back(new_session);
        }
        new_session->Start();
    }
    StartAccept();
}

void Server::RunIoService() {
    try {
        io_service_.run();
    } catch (std::exception& e) {
        LOG_ERROR(Server,
                this->port_<< " Exception in io_service thread: "<<e.what());
        std::exit(1);
    }
}

