#ifndef __RH_BOUNDEDBUFFER_H__
#define __RH_BOUNDEDBUFFER_H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/call_traits.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <cstring>
#include <iostream>

// Modified from: http://www.boost.org/doc/libs/1_60_0/libs/circular_buffer/test/bounded_buffer_comparison.cpp

template<class T>
class BoundedBuffer {
public:

	BoundedBuffer(size_t buffer_capacity) :
			buf(buffer_capacity), buf_capacity(buffer_capacity), buf_size(0), read_ptr(
					0), write_ptr(0) {
	}

	//blocking write - blocks when full
	size_t write(T* data, size_t size) {
		if (size == 0)
			return 0;
		boost::mutex::scoped_lock lock(m_mutex);
		m_not_full.wait(lock,
				boost::bind(&BoundedBuffer<T>::is_not_full, this));
		size = std::min(size, buf_capacity - buf_size);
		const size_t size_write1 = std::min(size, buf_capacity - write_ptr);
		memcpy(&buf[write_ptr], data, size_write1 * sizeof(T));
		memcpy(&buf[0], data + size_write1, (size - size_write1) * sizeof(T));
		update(write_ptr, size);
		buf_size += size;
		lock.unlock();
		m_not_empty.notify_one();
		return size;
	}

	//blocking read - blocks when empty
	size_t read(T* data, size_t size) {
		if (size == 0)
			return 0;
		boost::mutex::scoped_lock lock(m_mutex);
		m_not_empty.wait(lock,
				boost::bind(&BoundedBuffer<T>::is_not_empty, this));
		size = std::min(size, buf_size);
		const size_t size_read1 = std::min(size, buf_capacity - read_ptr);
		memcpy(data, &buf[read_ptr], size_read1 * sizeof(T));
		memcpy(data + size_read1, &buf[0], (size - size_read1) * sizeof(T));
		update(read_ptr, size);
		buf_size -= size;
		lock.unlock();
		m_not_full.notify_one();
		return size;
	}

	//non-blocking write
	size_t trywrite(T* data, size_t size) {
		if (size == 0)
			return 0;
		boost::mutex::scoped_lock lock(m_mutex);
		if (is_full())
			return 0;
		size = std::min(size, buf_capacity - buf_size);
		const size_t size_write1 = std::min(size, buf_capacity - write_ptr);
		memcpy(&buf[write_ptr], data, size_write1 * sizeof(T));
		memcpy(&buf[0], data + size_write1, (size - size_write1) * sizeof(T));
		update(write_ptr, size);
		buf_size += size;
		lock.unlock();
		m_not_empty.notify_one();
		return size;
	}

	//non-blocking read
	size_t tryread(T* data, size_t size) {
		if (size == 0)
			return 0;
		boost::mutex::scoped_lock lock(m_mutex);
		if (is_empty())
			return 0;
		size = std::min(size, buf_size);
		const size_t size_read1 = std::min(size, buf_capacity - read_ptr);
		memcpy(data, &buf[read_ptr], size_read1 * sizeof(T));
		memcpy(data + size_read1, &buf[0], (size - size_read1) * sizeof(T));
		update(read_ptr, size);
		buf_size -= size;
		lock.unlock();
		m_not_full.notify_one();
		return size;
	}

	size_t size() {
		boost::mutex::scoped_lock lock(m_mutex);
		return buf_size;
	}

	bool empty() {
		boost::mutex::scoped_lock lock(m_mutex);
		return buf_size == 0;
	}

	bool full() {
		boost::mutex::scoped_lock lock(m_mutex);
		return buf_size == buf_capacity;
	}

	size_t capacity() {
		boost::mutex::scoped_lock lock(m_mutex);
		return buf_capacity;
	}

	void dump() {
		boost::mutex::scoped_lock lock(m_mutex);
		if (buf_size == 0) {
			std::cout << "dump: empty!" << std::endl;
		} else if (write_ptr > read_ptr) {
			for (size_t ii = 0; ii < buf_size; ii++) {
				std::cout << "dump: bb[" << ii << "]=" << buf[read_ptr + ii]
						<< std::endl;
			}
		} else {
			size_t ii = 0;
			for (; read_ptr + ii < buf_capacity; ii++) {
				std::cout << "dump: bb[" << ii << "]=" << buf[read_ptr + ii]
						<< std::endl;
			}
			for (size_t jj = ii; jj - ii < write_ptr; jj++) {
				std::cout << "dump: bb[" << jj << "]=" << buf[jj - ii]
						<< std::endl;
			}
		}
	}

private:
	BoundedBuffer(const BoundedBuffer&);             // Disabled copy constructor.
	BoundedBuffer& operator =(const BoundedBuffer&); // Disabled assign operator.

	bool is_not_empty() const {
		return buf_size > 0;
	}

	bool is_empty() const {
		return buf_size == 0;
	}

	bool is_not_full() const {
		return buf_size < buf_capacity;
	}

	bool is_full() const {
		return buf_size >= buf_capacity;
	}

	void update(size_t& ptr, size_t size) {
		if (size >= buf_capacity - ptr)
			ptr = ptr + size - buf_capacity;
		else
			ptr = ptr + size;
	}

	std::vector<T> buf;
	size_t buf_capacity;
	size_t buf_size;
	size_t read_ptr;
	size_t write_ptr;

	boost::mutex m_mutex;
	boost::condition m_not_empty;
	boost::condition m_not_full;

};

#endif /* __RH_BOUNDEDBUFFER_H__ */

