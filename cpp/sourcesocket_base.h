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
#ifndef SOURCESOCKET_IMPL_BASE_H
#define SOURCESOCKET_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Resource_impl.h>
#include <ossie/ThreadedComponent.h>

#include <bulkio/bulkio.h>
#include "struct_props.h"

class sourcesocket_base : public Resource_impl, protected ThreadedComponent
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
        std::string connection_type;
        std::string ip_address;
        unsigned short port;
        std::string status;
        double total_bytes;
        float bytes_per_sec;
        CORBA::ULong max_bytes;
        CORBA::ULong min_bytes;
        unsigned short byte_swap;
        sri_struct sri;

        // Ports
        bulkio::OutOctetPort *dataOctet_out;
        bulkio::OutCharPort *dataChar_out;
        bulkio::OutShortPort *dataShort_out;
        bulkio::OutUShortPort *dataUshort_out;
        bulkio::OutULongPort *dataUlong_out;
        bulkio::OutLongPort *dataLong_out;
        bulkio::OutFloatPort *dataFloat_out;
        bulkio::OutDoublePort *dataDouble_out;

    private:
};
#endif // SOURCESOCKET_IMPL_BASE_H
