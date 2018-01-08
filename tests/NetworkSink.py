#!/usr/bin/env python
#
# This file is protected by Copyright. Please refer to the COPYRIGHT file distributed with this 
# source distribution.
# 
# This file is part of REDHAWK Basic Components sourcesocket.
# 
# REDHAWK Basic Components sourcesocket is free software: you can redistribute it and/or modify it under the terms of 
# the GNU Lesser General Public License as published by the Free Software Foundation, either 
# version 3 of the License, or (at your option) any later version.
# 
# REDHAWK Basic Components sourcesocket is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
# PURPOSE.  See the GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License along with this 
# program.  If not, see http://www.gnu.org/licenses/.
#
try:
    from bulkio.bulkioInterfaces import BULKIO as _BULKIO
    from bulkio.bulkioInterfaces import BULKIO__POA as _BULKIO__POA
except:
    # Handle case where bulkioInterface may not be installed
    pass

import ossie.utils.bulkio.bulkio_data_helpers as _bulkio_data_helpers
import logging as _logging

import Queue as _Queue
import socket
import struct
import threading as _threading
import time
import copy

from ossie.utils.sb import domainless as _domainless
from ossie.utils.sb import io_helpers

log = _logging.getLogger(__name__)

class NetworkSink(io_helpers._SinkBase):
    """
    Class to take in packets via BULKIO and push
    data through a TCP socket
    """

    def __init__(self):
        self.threadExited = None

        io_helpers._SinkBase.__init__(self)

        # Properties
        self.byte_swap       = 0
        self.bytes_per_sec   = 0
        self.connection_type = "server"
        self.ip_address      = ""
        self.port            = 32191
        self.total_bytes     = 0
        self.leftover        = ""
        # Internal Members
        self._dataQueue      = _Queue.Queue()
        self._dataSocket     = None
        self._runThread      = None
        self._serverSocket   = None

        self._openSocket()

    def _closeSockets(self):
        """
        Close the data and/or server sockets
        """
        if self._dataSocket:
            self._dataSocket.shutdown(socket.SHUT_RDWR)
            self._dataSocket.close()
            self._dataSocket = None

        if self._serverSocket:
            self._serverSocket.shutdown(socket.SHUT_RDWR)
            self._serverSocket.close()
            self._serverSocket = None

    def _flip(self,
             dataStr,
             numBytes):
        """
        Given data packed into a string, reverse bytes 
        for a given word length and return the 
        byte-flipped string
        """
        out = ""
    
        for i in xrange(len(dataStr)/numBytes):
            l = list(dataStr[numBytes*i:numBytes*(i+1)])
            l.reverse()
            out += (''.join(l))

        return out

    def _formatData(self,
                    data):
        """
        Given a list, or a list of lists, create a
        string representing the data
        """
        retval = None

        if str(type(data[0])) == "<type 'list'>":
            retval = ""

            for i in data:
                retval += append(self._listToString(i))

        else:
            retval = self._listToString(data)

        return retval

    def getPort(self, portName):
        if _domainless._DEBUG == True:
            print self.className + ":getPort() portName " + str(portName) + "================================="

        try:
            self._sinkPortType = self.getPortType(portName)

            # Set up output array sink
            if str(portName) == "xmlIn":
                self._sink = _bulkio_data_helpers.XmlArraySink(eval(self._sinkPortType))
            else:
                self._sink = _bulkio_data_helpers.ArraySink(eval(self._sinkPortType))

            if self._sink != None:
                self._sinkPortObject = self._sink.getPort()
                return self._sinkPortObject
            else:
                return None

        except Exception, e:
            log.error(self.className + ":getPort(): failed " + str(e))

        return None

    def _listToString(self,
                       listData):
        """
        Given a list, use the input port type to
        create a string representing the data
        """
        portType = self._sink.port_type

        if portType == _BULKIO__POA.dataChar:
            string = ''.join(listData)
        elif portType == _BULKIO__POA.dataOctet:
            string = ''.join(listData)
        elif portType == _BULKIO__POA.dataShort:
            string = struct.pack(str(len(listData)) + 'h', *listData)
        elif portType == _BULKIO__POA.dataUshort:
            string = struct.pack(str(len(listData)) + 'H', *listData)
        elif portType == _BULKIO__POA.dataLong:
            string = struct.pack(str(len(listData)) + 'i', *listData)
        elif portType == _BULKIO__POA.dataUlong:
            string = struct.pack(str(len(listData)) + 'I', *listData)
        elif portType == _BULKIO__POA.dataFloat:
            string = struct.pack(str(len(listData)) + 'f', *listData)
        elif portType == _BULKIO__POA.dataLongLong:
            string = struct.pack(str(len(listData)) + 'q', *listData)
        elif portType == _BULKIO__POA.dataUlongLong:
            string = struct.pack(str(len(listData)) + 'Q', *listData)
        elif portType == _BULKIO__POA.dataDouble:
            string = struct.pack(str(len(listData)) + 'd', *listData)
        elif portType == _BULKIO__POA.dataString:          
            string = listData[0]
        elif portType == _BULKIO__POA.dataXml:
            pass
        elif portType == _BULKIO__POA.dataFile:
            pass
        else:
            log.error("Invalid data type")
            string = None

        return string

    def _openSocket(self):
        """
        Open the data and/or server sockets
        based on the current properties
        """
        log.info("Connection Type: " + str(self.connection_type))
        log.info("IP Address: " + self.ip_address)
        log.info("Port: " + str(self.port))
        if self.connection_type == "server":
            self._dataSocket = None
            self._serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            try:
                self._serverSocket.bind(("localhost", self.port))
            except Exception, e:
                log.error("Unable to bind socket: " + str(e))
                return

            self._serverSocket.listen(1)
        elif self.connection_type == "client":
            self._dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._dataSocket.connect((self.ip_address, self.port))
            self._serverSocket = None
        else:
            log.error("Invalid connection type: " + self.connection_type)
            self._dataSocket = None
            self._serverSocket = None

    def _pushThread(self):
        """
        The thread function for collecting
        data from the sink and pushing it
        to the socket
        """
        self.settingsAcquired = False
        self.threadExited = False

        while not self._exitThread:
            if self._dataSocket == None:
                if self.connection_type == "server":
                    if self._serverSocket == None:
                        self._openSocket()
                    
                    log.debug("Waiting for client connection")
                    (self._dataSocket, clientAddress) = self._serverSocket.accept()
                    log.debug("Got client connection: " + str(clientAddress))
                else:
                    self._openSocket()

                time.sleep(0.1)
                continue

            if not self._sink:
                log.warn("No connections to NetworkSink")
                time.sleep(1.0)
                continue

            (retval, timestamps) = self._sink.retrieveData()

            if not retval or len(retval) == 0:
                time.sleep(0.1)
                continue
            data = self._formatData(retval)
            data=self.leftover+data
            self.leftover = ""

            # If the byte swap value is 1, then
            # use the size of the data
            if self.byte_swap == 1:
                portType = self._sink.port_type

                if portType == _BULKIO__POA.dataChar:
                    byteSwap = 1
                elif portType == _BULKIO__POA.dataOctet:
                    byteSwap = 1
                elif portType == _BULKIO__POA.dataShort:
                    byteSwap = 2
                elif portType == _BULKIO__POA.dataUshort:
                    byteSwap = 2
                elif portType == _BULKIO__POA.dataLong:
                    byteSwap = 4
                elif portType == _BULKIO__POA.dataUlong:
                    byteSwap = 4
                elif portType == _BULKIO__POA.dataFloat:
                    byteSwap = 4
                elif portType == _BULKIO__POA.dataLongLong:
                    byteSwap = 8
                elif portType == _BULKIO__POA.dataUlongLong:
                    byteSwap = 8
                elif portType == _BULKIO__POA.dataDouble:
                    byteSwap = 8
                elif portType == _BULKIO__POA.dataString:          
                    byteSwap = 1
                elif portType == _BULKIO__POA.dataXml:
                    pass
                elif portType == _BULKIO__POA.dataFile:
                    pass
                else:
                    byteSwap = 0
                
                if byteSwap != 0:
                    data = self._flip(data, byteSwap)

            elif self.byte_swap > 1:
                beforedata = copy.copy(data)
                data = self._flip(data, self.byte_swap)
                if len(data) < len(beforedata):
                    self.leftover = str(beforedata[len(data):])

            self._pushToSocket(data)

    def _pushToSocket(self,
                      data):
        """
        Push data to the current data socket,
        handling short writes as necessary
        """
        if self._dataSocket != None:
            dataSent = 0
            dataToSend = len(data)
            
            while dataSent != dataToSend:
                dataSentTemp = self._dataSocket.send(data[dataSent:])

                if dataSentTemp == -1:
                    log.error("Error with socket send")
                    break
                elif dataSentTemp == 0:
                    log.debug("Connection closed by remote host")
                    self._dataSocket.shutdown(socket.SHUT_RDWR)
                    self._dataSocket.close()
                    self._dataSocket = None
                else:
                    dataSent += dataSentTemp

    def releaseObject(self):
        self._closeSockets()

        io_helpers._SinkBase.releaseObject(self)

    def setConnection_type(self,
                           connection_type):
        """
        When this property changes, close the
        socket so it can be reopened with the
        new values
        """
        if connection_type != self.connection_type and (connection_type == "server" or connection_type == "client"):
            self.connection_type = connection_type
            self._closeSockets()

    def setIp_address(self,
                      ip_address):
        """
        When this property changes, close the
        socket so it can be reopened with the
        new values
        """
        if ip_address != self.ip_address:
            self.ip_address = ip_address
            self._closeSockets()

    def setPort(self,
                port):
        """
        When this property changes, close the
        socket so it can be reopened with the
        new values
        """
        if port != self.port:
            self.port = port
            self._closeSockets()

    def start(self):
        self._exitThread = False

        if self._runThread == None:
            self._runThread = _threading.Thread(target=self._pushThread)
            self._runThread.setDaemon(True)
            self._runThread.start()
        elif not self._runThread.isAlive():
            self._runThread = _threading.Thread(target=self._pushThread)
            self._runThread.setDaemon(True)
            self._runThread.start()

    def stop(self):
        if self._sink:
            self._sink.stop()

        self._exitThread = True

        if self.threadExited == None:
            timeout_count = 10
            while not self.threadExited:
                _time.sleep(0.1)
                timeout_count -= 1

                if timeout_count < 0:
                    raise AssertionError, self.className + ":stop() failed to exit thread"
