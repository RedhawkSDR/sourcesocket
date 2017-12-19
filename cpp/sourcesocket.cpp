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

/**************************************************************************

 This is the component code. This file contains the child class where
 custom functionality can be added to the component. Custom
 functionality to the base class can be extended here. Access to
 the ports can also be done from this class

 **************************************************************************/

#include "sourcesocket.h"

size_t gcd(size_t a, size_t b) {
    //find greatest common divisor using euclid's method
    if (b == 0)
        return a;
    return gcd(b, a % b);

}
size_t lcm(size_t a, size_t b) {
    //find least common multiple
    return a * b / (gcd(a, b));
}

PREPARE_LOGGING(sourcesocket_i)

sourcesocket_i::sourcesocket_i(const char *uuid, const char *label) :
        sourcesocket_base(uuid, label), server_(NULL), client_(NULL), sendNewSri(
                true) {
    port = 0; // the port Property does not have a default value, this will prevent it from being used before configured.
    data_length_ = 0;
    data_capacity_ = max_bytes;
    data_.resize(data_capacity_);
    theSri.hversion = 1;
    theSri.xunits = BULKIO::UNITS_TIME;
    theSri.subsize = 0;
    theSri.ystart = 0.0;
    theSri.ydelta = 0.0;
    theSri.yunits = BULKIO::UNITS_NONE;
    addPropertyChangeListener("byte_swap", this,
            &sourcesocket_i::byte_swapChanged);
    addPropertyChangeListener("connection_type", this,
            &sourcesocket_i::connection_typeChanged);
    addPropertyChangeListener("ip_address", this,
            &sourcesocket_i::ip_addressChanged);
    addPropertyChangeListener("max_bytes", this,
            &sourcesocket_i::max_bytesChanged);
    addPropertyChangeListener("min_bytes", this,
            &sourcesocket_i::min_bytesChanged);
    addPropertyChangeListener("port", this, &sourcesocket_i::portChanged);
    addPropertyChangeListener("sri", this, &sourcesocket_i::sriChanged);
    addPropertyChangeListener("internal_buffer_size", this,
            &sourcesocket_i::internal_buf_szChanged);
    addPropertyChangeListener("socket_settings", this,
            &sourcesocket_i::socket_settingsChanged);

    const sri_struct dummy;

    sriChanged(&dummy, (const sri_struct *) &theSri);
    status = "startup";
    total_bytes = 0;
    bytes_per_sec = 0;
}

void sourcesocket_i::start() throw (CF::Resource::StartError, CORBA::SystemException){
    updateXferLen();
    updateSocket(true);
    sourcesocket_base::start();

}

sourcesocket_i::~sourcesocket_i() {
    boost::recursive_mutex::scoped_lock lock(socketLock_);
    //status = "not_connected";
    if (server_)
        delete server_;
    if (client_)
        delete client_;
}

void sourcesocket_i::byte_swapChanged(const unsigned short *oldValue,
        const unsigned short *newValue) {
    if (*oldValue != *newValue) {
        boost::recursive_mutex::scoped_lock lock(xferLock_);
        updateXferLen();
    }
}

void sourcesocket_i::connection_typeChanged(const std::string *oldValue,
        const std::string *newValue) {
    if (*oldValue != *newValue) {
        if (started()) {
            boost::recursive_mutex::scoped_lock lock(socketLock_);
            updateSocket();
        }
    }
}

void sourcesocket_i::ip_addressChanged(const std::string *oldValue,
        const std::string *newValue) {
    if (*oldValue != *newValue) {
        if (started()) {
            boost::recursive_mutex::scoped_lock lock(socketLock_);
            updateSocket();
        }
    }
}

void sourcesocket_i::max_bytesChanged(const CORBA::ULong *oldValue,
        const CORBA::ULong *newValue) {
    if (*oldValue != *newValue) {
        boost::recursive_mutex::scoped_lock xferlock(xferLock_);
        updateXferLen();
    }
}

void sourcesocket_i::min_bytesChanged(const CORBA::ULong *oldValue,
        const CORBA::ULong *newValue) {
    if (*oldValue != *newValue) {
        boost::recursive_mutex::scoped_lock lock(xferLock_);
        updateXferLen();
    }
}

void sourcesocket_i::portChanged(const unsigned short *oldValue,
        const unsigned short *newValue) {
    if (*oldValue != *newValue) {
        if (*newValue < 1024) {
            LOG_WARN(sourcesocket_i,
                    "Configured port value " << *newValue << ". Ports below 1024 are reserved for privileged users (i.e. root).");
        }
        if (started()) {
            boost::recursive_mutex::scoped_lock lock(socketLock_);
            updateSocket();
        }
    }
}

void sourcesocket_i::internal_buf_szChanged(const CORBA::ULong *oldValue,
        const CORBA::ULong *newValue) {
    if (*oldValue != *newValue) {
        if (started()) {
            boost::recursive_mutex::scoped_lock lock(socketLock_);
            updateSocket();
        }
    }
}

void sourcesocket_i::socket_settingsChanged(const socket_settings_struct *oldValue,
        const socket_settings_struct *newValue) {
    if (*oldValue != *newValue && newValue->enable) {
        if (started()) {
            boost::recursive_mutex::scoped_lock lock(socketLock_);
            updateSocket();
        }
    }
}

void sourcesocket_i::sriChanged(const sri_struct *oldValue,
        const sri_struct *newValue) {
    boost::recursive_mutex::scoped_lock lock(socketLock_);

    if (oldValue != newValue) {
        if (sri.streamID.empty())
            sri.streamID = ossie::generateUUID();
        theSri.hversion = 1;
        theSri.xstart = sri.xstart;
        theSri.xdelta = sri.xdelta;
        theSri.mode = sri.mode;
        theSri.streamID = sri.streamID.c_str();
        theSri.blocking = sri.blocking;
        sendNewSri = true;
    }
}

int sourcesocket_i::serviceFunction() {
    //cash off max_bytes & min_bytes in case their properties are updated mid service function
    unsigned int maxBytes;
    unsigned int minBytes;
    unsigned int byteSwap;
    unsigned int multSize;
    activePorts_.clear();
    {
        boost::recursive_mutex::scoped_lock lock(xferLock_);
        maxBytes = max_bytes;
        minBytes = min_bytes;
        byteSwap = byte_swap;
        multSize = multSize_;
    }

    std::string streamID(theSri.streamID._ptr);

    //send out data if we have more than we should
    //loop until we have less than max_bytes left
    //this should only be called if max_bytes was DECREASED since last loop
    if (sendNewSri) {
        dataOctet_out->pushSRI(theSri);
        dataChar_out->pushSRI(theSri);
        dataUshort_out->pushSRI(theSri);
        dataShort_out->pushSRI(theSri);
        dataUlong_out->pushSRI(theSri);
        dataLong_out->pushSRI(theSri);
        dataDouble_out->pushSRI(theSri);
        dataFloat_out->pushSRI(theSri);
        sendNewSri = false;
    }

    if (data_length_ >= maxBytes) {
        size_t numLoops = data_length_ / maxBytes;
        for (size_t i = 0; i != numLoops; i++) {
            pushData<bulkio::OutOctetPort, CORBA::Octet>(dataOctet_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutCharPort, signed char>(dataChar_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutUShortPort, CORBA::UShort>(dataUshort_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutShortPort, CORBA::Short>(dataShort_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutULongPort, CORBA::ULong>(dataUlong_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutLongPort, CORBA::Long>(dataLong_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutDoublePort, CORBA::Double>(dataDouble_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
            pushData<bulkio::OutFloatPort, CORBA::Float>(dataFloat_out,
                    &data_[i * maxBytes], maxBytes, byteSwap);
        }
        if ((data_length_ -= numLoops * maxBytes) > 0)
            memcpy(&data_[0], &data_[numLoops * maxBytes], data_length_); // src/dest will not overlap, memcpy ok to use here.

    }
    if (data_capacity_ != maxBytes) {
        data_capacity_ = maxBytes;
        data_.resize(data_capacity_);
    }

    boost::recursive_mutex::scoped_lock lock(socketLock_);

    size_t numRead = 0;
    LOG_DEBUG(sourcesocket_i, __PRETTY_FUNCTION__ << " before read: maxBytes=" << maxBytes
                                          << " sock_read_buf_.size()=" << data_.size()
                                          << " data_length_=" << data_length_
                                          << " data_capacity_=" << data_capacity_);
    try {
        if (server_) {
            numRead = server_->Read(&data_[data_length_],
                    data_capacity_ - data_length_);
            data_length_ += numRead;
        } else if (client_) {
            numRead = client_->Read(&data_[data_length_],
                    data_capacity_ - data_length_);
            data_length_ += numRead;
        } else {
            updateSocket();
        }

        if (numRead > 0) {
            // Report connected when receiving data, even though connection may have dropped
            // if connection drops, we'll eat through the data and report correct status soon enough
            status = "connected";
        } else {
            // For performance, only do this when not receiving data
            if (server_ && server_->is_connected()) {
                status = "connected";
            } else if (client_ && client_->Connect()) {
                status = "connected";
            } else {
                status = "not_connected";
            }
        }

    } catch (std::exception& e) {
        LOG_ERROR(sourcesocket_i, "Exception caught: " << e.what());
        throw(e);
    }
    LOG_DEBUG(sourcesocket_i,
            "Received " << numRead<< " bytes, max size = " << maxBytes);
    LOG_DEBUG(sourcesocket_i, __PRETTY_FUNCTION__ << " after read: maxBytes=" << maxBytes
                                          << " sock_read_buf_.size()=" << data_.size()
                                          << " data_length_=" << data_length_
                                          << " data_capacity_=" << data_capacity_);

    bytes_per_sec = stats_.newPacket(numRead);
    total_bytes += numRead;

    if (data_length_ >= minBytes || (numRead == 0 && data_length_ >= multSize && status == "not connected")) {
        // if have enough to push, OR...
        // if no new data, AND have at least 1 full sample, AND not connected...
        // push out what we can
        // TODO - [when] should "numLeft" be discarded? ever?
        size_t numLeft = data_length_ % multSize;
        size_t pushBytes = data_length_ - numLeft;

        pushData<bulkio::OutOctetPort, CORBA::Octet>(dataOctet_out, &data_[0],
                pushBytes, byteSwap);
        pushData<bulkio::OutCharPort, signed char>(dataChar_out, &data_[0],
                pushBytes, byteSwap);
        pushData<bulkio::OutUShortPort, CORBA::UShort>(dataUshort_out,
                &data_[0], pushBytes, byteSwap);
        pushData<bulkio::OutShortPort, CORBA::Short>(dataShort_out, &data_[0],
                pushBytes, byteSwap);
        pushData<bulkio::OutULongPort, CORBA::ULong>(dataUlong_out, &data_[0],
                pushBytes, byteSwap);
        pushData<bulkio::OutLongPort, CORBA::Long>(dataLong_out, &data_[0],
                pushBytes, byteSwap);
        pushData<bulkio::OutDoublePort, CORBA::Double>(dataDouble_out,
                &data_[0], pushBytes, byteSwap);
        pushData<bulkio::OutFloatPort, CORBA::Float>(dataFloat_out, &data_[0],
                pushBytes, byteSwap);

        if ((data_length_ = numLeft) > 0)
            memcpy(&data_[0], &data_[pushBytes], data_length_);

        if (activePorts_.size() > 1) {
            std::string warnstr("More than one port is active: ");
            for (std::vector<std::string>::iterator i = activePorts_.begin();
                    i != activePorts_.end(); i++)
                warnstr += *i + " ";
            LOG_WARN(sourcesocket_i, warnstr + ".");
        }
        LOG_DEBUG(sourcesocket_i, __PRETTY_FUNCTION__ << " after push: maxBytes=" << maxBytes
                                              << " sock_read_buf_.size()=" << data_.size()
                                              << " data_length_=" << data_length_
                                              << " data_capacity_=" << data_capacity_);
        return NORMAL;
    }
    LOG_DEBUG(sourcesocket_i, __PRETTY_FUNCTION__ << " NO push: maxBytes=" << maxBytes
                                          << " sock_read_buf_.size()=" << data_.size()
                                          << " data_length_=" << data_length_
                                          << " data_capacity_=" << data_capacity_
                                          << " minBytes=" << minBytes
                                          << " numRead=" << numRead
                                          << " multSize=" << multSize
                                          << " status=" << status);
    return NOOP;

}

void sourcesocket_i::updateXferLen() {
    //Adjust the key properties dealing with i/o in here
    //ENSURE THE FOLLOWING GOALS:
    //1.  multSize is an even number of the byte_swap and the size of double
    //so we can always output a multiple number of properly swapped elements
    //2.  max_bytes is a multiple of multSize less than (or equal) what the user has requested
    //3.  min_bytes is a multiple of the multSize greater than (or equal to) what the user has requested

    if (byte_swap > 1)
        multSize_ = lcm(sizeof(double), byte_swap);
    else
        multSize_ = sizeof(double);

    if (max_bytes < multSize_)
        max_bytes = multSize_;
    if (min_bytes < 1)
        min_bytes = 1;

    max_bytes = max_bytes - max_bytes % multSize_;
    if (min_bytes > max_bytes)
        min_bytes = max_bytes;
    else
        min_bytes = (min_bytes + multSize_ - 1)
                - ((min_bytes + multSize_ - 1) % multSize_);
}

void sourcesocket_i::updateSocket(bool starting) {

    if (client_) {
        delete client_;
        client_ = NULL;
    }
    if (server_) {
        delete server_;
        server_ = NULL;
    }
    if (!started() && !starting) {
        return;
    }

    if (connection_type == "server" && port > 0) {
        try {
            if (socket_settings.enable) {
                server_ = new Server(port, internal_buffer_size, socket_settings.receive_buffer_size,
                        socket_settings.tcp_nodelay);
            } else {
                server_ = new Server(port, internal_buffer_size);
            }
            if (server_->is_connected())
                status = "connected";
            else
                status = "not_connected";
        } catch (std::exception& e) {
            if (server_) {
                delete server_;
                server_ = NULL;
            }
            LOG_ERROR(sourcesocket_i,
                    "Exception starting server on port " << port << ": " << e.what());
        }
        LOG_INFO(sourcesocket_i, "Set as SERVER :" << port);
    } else if (connection_type == "client" && port > 0 && !ip_address.empty()) {
        try {
            if (socket_settings.enable) {
                client_ = new Client(port, ip_address, internal_buffer_size, socket_settings.receive_buffer_size,
                        socket_settings.tcp_nodelay);
            } else {
                client_ = new Client(port, ip_address, internal_buffer_size);
            }
            if (client_->Connect()) {
                status = "connected";
                LOG_INFO(sourcesocket_i,
                        "Connected as CLIENT " << ip_address << ":" << port);
            } else {
                status = "not_connected";
            }
            LOG_INFO(sourcesocket_i,
                    "Set as CLIENT " << ip_address << ":" << port);
        } catch (std::exception& e) {
            LOG_ERROR(sourcesocket_i, "Exception starting client: " << e.what());
        }
    } else {
        LOG_ERROR(sourcesocket_i,
                    "Started with bad connection parameters - " << connection_type << " " << ip_address << ":" << port);
    }
}
