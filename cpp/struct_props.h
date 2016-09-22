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
 
#ifndef STRUCTPROPS_H
#define STRUCTPROPS_H

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

*******************************************************************************************/

#include <ossie/CorbaUtils.h>
#include <CF/cf.h>
#include <ossie/PropertyMap.h>

struct socket_settings_struct {
    socket_settings_struct ()
    {
        enable = false;
        receive_buffer_size = 65536;
        tcp_nodelay = false;
    };

    static std::string getId() {
        return std::string("socket_settings");
    };

    bool enable;
    CORBA::ULong receive_buffer_size;
    bool tcp_nodelay;
};

inline bool operator>>= (const CORBA::Any& a, socket_settings_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("socket::enable")) {
        if (!(props["socket::enable"] >>= s.enable)) return false;
    }
    if (props.contains("socket::receive_buffer_size")) {
        if (!(props["socket::receive_buffer_size"] >>= s.receive_buffer_size)) return false;
    }
    if (props.contains("socket::tcp_nodelay")) {
        if (!(props["socket::tcp_nodelay"] >>= s.tcp_nodelay)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const socket_settings_struct& s) {
    redhawk::PropertyMap props;
 
    props["socket::enable"] = s.enable;
 
    props["socket::receive_buffer_size"] = s.receive_buffer_size;
 
    props["socket::tcp_nodelay"] = s.tcp_nodelay;
    a <<= props;
}

inline bool operator== (const socket_settings_struct& s1, const socket_settings_struct& s2) {
    if (s1.enable!=s2.enable)
        return false;
    if (s1.receive_buffer_size!=s2.receive_buffer_size)
        return false;
    if (s1.tcp_nodelay!=s2.tcp_nodelay)
        return false;
    return true;
}

inline bool operator!= (const socket_settings_struct& s1, const socket_settings_struct& s2) {
    return !(s1==s2);
}

struct sri_struct {
    sri_struct ()
    {
        xstart = 0.0;
        xdelta = 1.0;
        mode = 0;
        streamID = "default_stream_id";
        blocking = true;
    };

    static std::string getId() {
        return std::string("sri");
    };

    double xstart;
    double xdelta;
    short mode;
    std::string streamID;
    bool blocking;
};

inline bool operator>>= (const CORBA::Any& a, sri_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("xstart")) {
        if (!(props["xstart"] >>= s.xstart)) return false;
    }
    if (props.contains("xdelta")) {
        if (!(props["xdelta"] >>= s.xdelta)) return false;
    }
    if (props.contains("mode")) {
        if (!(props["mode"] >>= s.mode)) return false;
    }
    if (props.contains("streamID")) {
        if (!(props["streamID"] >>= s.streamID)) return false;
    }
    if (props.contains("blocking")) {
        if (!(props["blocking"] >>= s.blocking)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const sri_struct& s) {
    redhawk::PropertyMap props;
 
    props["xstart"] = s.xstart;
 
    props["xdelta"] = s.xdelta;
 
    props["mode"] = s.mode;
 
    props["streamID"] = s.streamID;
 
    props["blocking"] = s.blocking;
    a <<= props;
}

inline bool operator== (const sri_struct& s1, const sri_struct& s2) {
    if (s1.xstart!=s2.xstart)
        return false;
    if (s1.xdelta!=s2.xdelta)
        return false;
    if (s1.mode!=s2.mode)
        return false;
    if (s1.streamID!=s2.streamID)
        return false;
    if (s1.blocking!=s2.blocking)
        return false;
    return true;
}

inline bool operator!= (const sri_struct& s1, const sri_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
