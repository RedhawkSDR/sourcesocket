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
#ifndef SOURCESOCKET_BASE_IMPL_BASE_H
#define SOURCESOCKET_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Component.h>
#include <ossie/ThreadedComponent.h>

#include <bulkio/bulkio.h>
#include "struct_props.h"

class sourcesocket_base : public Component, protected ThreadedComponent
{
    public:
        sourcesocket_base(const char *uuid, const char *label);
        ~sourcesocket_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();

    protected:
        // Member variables exposed as properties
        /// Property: connection_type
        std::string connection_type;
        /// Property: ip_address
        std::string ip_address;
        /// Property: port
        unsigned short port;
        /// Property: status
        std::string status;
        /// Property: total_bytes
        double total_bytes;
        /// Property: bytes_per_sec
        float bytes_per_sec;
        /// Property: max_bytes
        CORBA::ULong max_bytes;
        /// Property: min_bytes
        CORBA::ULong min_bytes;
        /// Property: byte_swap
        unsigned short byte_swap;
        /// Property: internal_buffer_size
        CORBA::ULong internal_buffer_size;
        /// Property: socket_settings
        socket_settings_struct socket_settings;
        /// Property: sri
        sri_struct sri;

        // Ports
        /// Port: dataOctet_out
        bulkio::OutOctetPort *dataOctet_out;
        /// Port: dataChar_out
        bulkio::OutCharPort *dataChar_out;
        /// Port: dataShort_out
        bulkio::OutShortPort *dataShort_out;
        /// Port: dataUshort_out
        bulkio::OutUShortPort *dataUshort_out;
        /// Port: dataUlong_out
        bulkio::OutULongPort *dataUlong_out;
        /// Port: dataLong_out
        bulkio::OutLongPort *dataLong_out;
        /// Port: dataFloat_out
        bulkio::OutFloatPort *dataFloat_out;
        /// Port: dataDouble_out
        bulkio::OutDoublePort *dataDouble_out;

    private:
};
#endif // SOURCESOCKET_BASE_IMPL_BASE_H
