#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_EIBConnection.py
#Version: V0.1 , 28.08.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

#from FB_EIB.FB_EIBFrameListener import FB_EIBFrameListener
from Global import Global
import socket
import jpype

import gtk

class FB_EIBConnection(object):#FB_EIBFrameListener):

    _individualAddress = "0.0.0"
    _connected = False

    __name = ""                 #name of connection
    __type = 0                  #init Connectiontype
    __isDefault = False         #Defaultconnection
    __COMPort = 0               #index of combo 0 = COM 1
    __ipName = ""
    __ipIP = "192.168.10.1"
    __ipPort = 3671
    __id = 0

    __KNXConnection = None        #calimero connection
    __KNXNetworkLinkIP = None     #calimero Link object according to the KNXCOnnection


    __parent = None

    ##Constructor for EIBConnection.
    def __init__(self):
        self.__name = "neue Verbindung"
        self.__isDefault = True
        self.__id = Global.CreateGUID()
        self.__parent = self

    def getID(self):
        return self.__id

    ##set the name of the new connection
    #@param name: new name
    def setName(self,name):
        self.__name = name

    ##get the name of this connection
    #@return: name
    def getName(self):
        return self.__name

    ##set the type of the connection
    #@param type: index of the typebox, corresponding with enumeration
    def setType(self,type):
        self.__type = type

    ##get the type of this connection
    #@return: index of type / enum.value
    def getType(self):
        return self.__type

    ##set the flag as default connection
    #@param isDefault: connection is default connection
    def setDefault(self,isDefault):
        self.__isDefault = isDefault

    ##get the flag to indicate the default connection
    #@return: default-flag
    def getDefault(self):
        return self.__isDefault

    ##set the serial comport
    #@param ComPort:
    def setCOM(self,ComPort):
        self.__COMPort = ComPort

    ##get the enum-value of the comport
    #@return:
    def getCOM(self):
        return self.__COMPort

    #---------------------------------------------------------------
    ##set (host)name of the ip-connection
    #@param ipName:
    def setIpName(self,ipName):
        self.__ipName = ipName

    ##get the hostname of the ip-connection
    #@return:
    def getIpName(self):
        return self.__ipName

    ##set the ip of the ip-connection
    #@param ipName:
    def setIP(self,ip):
        self.__ipIP = ip

    ##get the ip of the ip-connection
    #@return:
    def getIP(self):
        return self.__ipIP

    ##set the port-number of the ip-connection
    #@param port:
    def setIPPort(self,port):
        self.__ipPort = port

    ##get the port-number of the ip-connection
    #@return:
    def getIPPort(self):
        return self.__ipPort

#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------

    def doConnect(self):
        if self.__type == 0:    #EIBNet/IP
            return self.ConnectIP()
        elif self.__type == 1:    #RS232 FT1.2
            return False

    def doDisconnect(self):
        try:
            self._connected = False
            self.__KNXConnection.close()
            self.__KNXConnection = None
            self.__KNXNetworkLinkIP = None
        except:
            pass

    def ConnectIP(self):
        try:
            #create a socket
            RValue = False
            self._connected = False
            #s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            hostname = socket.gethostname()
            localIP = socket.gethostbyname(hostname)
            remoteIP = self.__ipIP

            localEP = Global.InetSocketAddress(localIP,0)
            remoteEP = Global.InetSocketAddress(remoteIP,int(self.__ipPort))

            self.__KNXNetworkLinkIP = Global.KNXNetworkLinkIP(Global.KNXNetworkLinkIP.TUNNEL,localEP,remoteEP,False,Global.TPSettings(True))
            #ip tunnel connection
            self.__KNXConnection = Global.KNXnetIPTunnel(Global.KNXnetIPTunnel.LINK_LAYER,localEP,remoteEP,False)

            #create a JProxy to integrate the Java interface KNXListener
            d = {
            'frameReceived' : self.frameReceived,
            'connectionClosed' : self.connectionClosed,
            }
            proxy = jpype.JProxy(Global.KNXListener, dict=d)
            self.__KNXConnection.addConnectionListener(proxy)


            if self.__KNXConnection.getState() == Global.KNXnetIPConnection.OK:
                RValue = True
                self._connected = True
            else:
                RValue = False
                self._connected = False
            return RValue

        except jpype.JavaException, ex :
            error = ""
            self._connected = False

            if jpype.JavaException.javaClass(ex) is Global.KNXException:
                error = "Allg. Socket-Verbindungsfehler : " + str(jpype.JavaException.message(ex))
            elif jpype.JavaException.javaClass(ex) is Global.KNXTimeoutException:
                error = "Fehler beim Verbindungsaufbau : " + str(jpype.JavaException.message(ex))
            elif jpype.JavaException.javaClass(ex) is Global.KNXRemoteException:
                error = "Fehler beim Remote-Server : " + str(jpype.JavaException.message(ex))
            elif jpype.JavaException.javaClass(ex) is Global.KNXInvalidResponseException:
                error = "Falsches Format bei der Antwort : " + str(jpype.JavaException.message(ex))

            msgbox = gtk.MessageDialog(parent = self.__parent, buttons = gtk.BUTTONS_OK,
                                           flags = gtk.DIALOG_MODAL, type = gtk.MESSAGE_ERROR,
                                           message_format = error )

            msgbox.set_title(Global.ERRORCONNECTIONTITLE)
            result = msgbox.run()
            msgbox.destroy()

#------------------------------------------------------------------------------------------------
#--------------------------------- Events from Calimero------------------------------------------
#------------------------------------------------------------------------------------------------


    def frameReceived(self,FrameEvent):
        EIBFrame = FrameEvent.getFrame()
        FrameByte = FrameEvent.getFrameBytes()


    def connectionClosed(self,CloseEvent):
        #print CloseEvent.getReason()
        self._connected = False


#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------------


    ##Call this method to add an EIBframeListener for receiving a frame
    ##@param efl the EIBFrameListener
    #def addEIBFrameListener(self, efl):
        #check for double listeners
     #   for cnt in range(len(self.listeners)):
      #      if listeners[cnt] == efl:
       #         return
        #self._listeners.append(efl)

    ##Remove a EIBFrameListener
    #@param efl the instance of the Listener you want to remove
    #def removeEIBFrameListener(self,efl):
     #   for cnt in range(len(self.listeners)):
      #     if listeners[cnt] == efl:
       #        del listeners[cnt]
        #       return

    ##Returns the individual (physical) address of the EIB interface
    #@return the individual (physical) address of the EIB interface
    def getIndividualAddress(self):
        return self._individualAddress

    ##returns the KNXNetworkLink according to the current connection
    def getKNXNetworkLink(self):
        return self.__KNXNetworkLinkIP


    def isConnected(self):
        return self._connected

    def setConnected(self,connected):
        self._connected = connected


