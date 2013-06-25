/*
 * BoostServer.h
 *
 *  Created on: Jun 14, 2013
 *      Author: bsg
 */

#ifndef BOOSTSERVER_H_
#define BOOSTSERVER_H_

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread.hpp>
#include <boost/asio/error.hpp>
#include <deque>

using boost::asio::ip::tcp;

class server;

class session
{
public:
	session(boost::asio::io_service& io_service, server* s, size_t max_length)
	: socket_(io_service),
	  server_(s),
	  read_data_(max_length),
	  max_length_(max_length)
	{
	}

	tcp::socket& socket()
	{
		return socket_;
	}

	void start();

	template<typename T, typename U>
	void write(std::vector<T, U>& data);



private:
	void handle_read(const boost::system::error_code& error,
			size_t bytes_transferred);


	void handle_write(const boost::system::error_code& error);


	tcp::socket socket_;
	server* server_;
	std::vector<char> read_data_;
	size_t max_length_;
	std::deque<std::vector<char> > writeBuffer_;
	boost::mutex writeLock_;

};

class server
{
public:
	server(short port, size_t maxLength=1024) :
		acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)),
		thread_(NULL),
		pending_(NULL),
		maxLength_(maxLength)
	{
		start_accept();
		thread_ = new boost::thread(boost::bind(&server::run, this));
	}

	~server()
	{
		{
			boost::mutex::scoped_lock lock(sessionsLock_);
			for (std::list<session*>::iterator i=sessions_.begin(); i!= sessions_.end(); i++)
			{
				delete *i;
			}
			sessions_.clear();
		}
		{
			boost::mutex::scoped_lock lock(pendingLock_);
			if (pending_)
			{
				delete pending_;
				pending_=NULL;
			}
		}
		if(thread_)
		{
			io_service_.stop();
			thread_->join();
			delete thread_;
		}
	}

	template<typename T, typename U>
	void write(std::vector<T, U>& data);
	template<typename T>
	void read(std::vector<char, T> & data, size_t index=0);
	bool is_connected();

	template<typename T>
	void newSessionData(std::vector<char, T>& data);
	void closeSession(session* ptr);


private:
	void start_accept();
	void handle_accept(session* new_session,
			const boost::system::error_code& error);

	void run();

	boost::asio::io_service io_service_;
	tcp::acceptor acceptor_;
	std::list<session*> sessions_;
	std::vector<char> pendingData_;
	boost::mutex sessionsLock_;
	boost::mutex pendingDataLock_;
	boost::mutex pendingLock_;
	boost::thread* thread_;
	session* pending_;
	size_t maxLength_;
};


#endif /* BOOSTSERVER_H_ */
