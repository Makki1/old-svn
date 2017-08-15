#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_ProgramFrame.py
#Version: V0.1 , 29.08.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
import sys
import time
#import thread
#import Queue
#import threading
#import thread
import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade
import pickle

import jpype
import thread

from Global import Global
from GUI import FB_DlgConnectionManager

class FB_ProgramFrame(object):

    __curProject = None            #project object
    __cbConnections = None         #widget combo connections
    __bConnect = None              #widget connect button

    __parentClass = None            #object of its own class

    __curConnectionInstance = None  #instance of the current connection (FB_EIBConnection)

    #Devices in programming mode
    __ListViewProgDevices = None    #widget Tree/Listview to show devices in programming mode
    __CheckTimer = None             #timer object for check devices in cycle
    __toggleCheckProgDevices = None

    def __init__(self,curProject):

        self.__parentClass = self
        self.__curProject = curProject

        GladeObj = gtk.glade.XML(Global.GUIPath + Global.GladeFile,"winProgramming")

        dic = { "on_bConnectionConfig_clicked":self.ShowConnectionManager ,
                "on_bTestConnection_clicked":self.ClickTestConnection,
                "on_bConnect_toggled":self.ToggleConnect,
                "on_cbConnections_changed":self.ConnectionsChanged,

                "on_toggleCheckProgDevices_toggled":self.ToggleCheckProgDevices,
               }
        GladeObj.signal_autoconnect(dic)

        #read widgets
        self.__cbConnections = GladeObj.get_widget("cbConnections")
        self.__bConnect = GladeObj.get_widget("bConnect")
        self.__ListViewProgDevices = GladeObj.get_widget("ListViewProgDevices")
        self.__toggleCheckProgDevices = GladeObj.get_widget("toggleCheckProgDevices")

        #init model combobox to show connections
        liststore = gtk.ListStore(str,str)    #just one string at first..., 2nd string for GUID
        self.__cbConnections.set_model(liststore)
        self.text_cell = gtk.CellRendererText()
        self.__cbConnections.pack_start(self.text_cell,True)
        self.__cbConnections.add_attribute(self.text_cell, "text", 0)

        #init model tree/listview to show devices in progmode
        liststore = gtk.ListStore(gtk.gdk.Pixbuf, str)
        self.__ListViewProgDevices.set_model(liststore)
        self.text_cell = gtk.CellRendererText()            #Text Object
        self.img_cell = gtk.CellRendererPixbuf()           #Image Object
        self.column = gtk.TreeViewColumn()
        self.column.pack_start(self.img_cell, False)
        self.column.pack_start(self.text_cell,True)
        self.column.add_attribute(self.img_cell, "pixbuf",0)
        self.column.add_attribute(self.text_cell, "text", 1)
        self.column.set_attributes(self.text_cell, markup=1)
        self.__ListViewProgDevices.append_column(self.column)

        #init timer to check devices in progmode
        #self.__CheckTimer = threading.Timer(5.0, self.ReadDevicesInProgMode)


        self.LoadConnectionFromDB()
        self.UpdateUserConnections()
        winProgramming = GladeObj.get_widget("winProgramming")
        winProgramming.show()



    #Dialog: Connection-Manager
    def ShowConnectionManager(self,widget, data=None):
       FB_DlgConnectionManager.FB_DlgConnectionManager(self.__curProject, self.__parentClass)

    #button: Test-Connection
    #open the current connection and test it...
    def ClickTestConnection(self,widget, data=None):
         pass

    def ToggleConnect(self,widget, data=None):

        model = self.__cbConnections.get_model()
        iter = self.__cbConnections.get_active_iter()
        id = model.get_value(iter,1)
        self.__curConnectionInstance = self.getEIBConnection(id)

        if widget.get_active() == True:
            #connect
            self.__curConnectionInstance.doConnect()
        else:
            #disconnect
            self.__curConnectionInstance.doDisconnect()

        self.SetConnectButtonState(widget)

    #callback change combo connections
    def ConnectionsChanged(self,widget, data=None):
        #disconnect in case of changing the connection
        if self.__curConnectionInstance <> None:
            self.__curConnectionInstance.doDisconnect()
            self.SetConnectButtonState(self.__bConnect)


    def SetConnectButtonState(self,widget):
        if self.__curConnectionInstance.isConnected() == True:
            widget.set_active(True)
            widget.set_label("Verbunden")
        else:
            widget.set_active(False)
            widget.set_label("Verbinden")

    #gets the instance of a FB_EIBConnection with the given id
    def getEIBConnection(self,id):
        RValue = None

        if self.__curProject <> None:
            if self.__curProject.eibConnectionList <> None:
                for i in range(len(self.__curProject.eibConnectionList)):
                    if id == self.__curProject.eibConnectionList[i].getID():
                        RValue = self.__curProject.eibConnectionList[i]
                        break
        return RValue

    ##function to update the combobox in parentframe to show/select for user
    #@param cbConnections: widget of the combobox in parentframe which should be loaded
    def UpdateUserConnections(self):

        try:
            #copy list in combo connections in program_Frame (parent)
            if(self.__curProject <> None):# and self._MyConnection <> None):
                model = self.__cbConnections.get_model()
                #save id of the current connection / which is currently selected
                curIter = self.__cbConnections.get_active_iter()
                if curIter <> None:
                    idsaved = model.get_value(curIter,1)    #column 1 = id
                else:
                    idsaved = 0

                model.clear()

                IterSaved = None        #init Iterator

                for i in range(len(self.__curProject.eibConnectionList)):
                    Name = self.__curProject.eibConnectionList[i].getName()
                    typeID = self.__curProject.eibConnectionList[i].getType()
                    Type = str(Global.ConTypesText[typeID])

                    id = self.__curProject.eibConnectionList[i].getID()

                    tmp = Name + " mit '" + Type + "'"
                    iter = model.append([tmp, id])
                    #look if saved id is still in list and set this item to the active item
                    if idsaved == id:
                        IterSaved = iter

                #connection still existing...
                if IterSaved <> None:
                    self.__cbConnections.set_active_iter(IterSaved)
                else:
                    if len(self.__curProject.eibConnectionList) > 0:
                        self.__cbConnections.set_active(0)

            else:
                #no connections in list or no valid project is loaded
                model = self.__cbConnections.get_model()
                model.clear()
        except:
            pass

    def LoadConnectionFromDB(self):
        #try:
            cursor = Global.DatabaseConnection.cursor()
            cursor.execute("SELECT * FROM Connections")

            del self.__curProject.eibConnectionList[0:len(self.__curProject.eibConnectionList)]
            for row in cursor:
                tmpCon = pickle.loads(row[2])    #column 2 contains class data
                self.__curProject.eibConnectionList.append(tmpCon)
       #except:
           # pass
#---------------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------------
    ##button to start reading Devices in progmode
    ##
    def ToggleCheckProgDevices(self,widget,Data=None):

        if widget.get_active() == True:
            widget.set_label("zyklischer Suchlauf...")
            self.ReadDevicesInProgMode()
            #self.__CheckTimer.start()
        else:
            widget.set_label("Suchlauf starten")
            #self.__CheckTimer.cancel()

#---------------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------------
#section physical addresses

    def ReadDevicesInProgMode(self):
        #read the PA of devices in programming mode
        try:
            mngClient = Global.ManagementClientImpl(self.__curConnectionInstance.getKNXNetworkLink())
            IndivAddrList = mngClient.readAddress(False)

            model = self.__ListViewProgDevices.get_model()
            model.clear()
            image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "Device.png")
            for Addr in IndivAddrList:
                Iterator = model.append([image,Addr.toString()])

        except jpype.JavaException, ex :
            error = ""
            if jpype.JavaException.javaClass(ex) is Global.KNXTimeoutException:
                error = U"keine Geräte im Programmiermodus : " + str(jpype.JavaException.message(ex))
            elif jpype.JavaException.javaClass(ex) is Global.KNXInvalidResponseException :
                error = U"ungültige Antwort beim Lesen der Addressen : " + str(jpype.JavaException.message(ex))
            elif jpype.JavaException.javaClass(ex) is Global.KNXLinkClosedException:
                error = U"kein geöffneter Netzwerk-Link : " + str(jpype.JavaException.message(ex))
            elif jpype.JavaException.javaClass(ex) is Global.KNXRemoteException:
                error = U"Fehler beim Remote-Server : " + str(jpype.JavaException.message(ex))

            msgbox = gtk.MessageDialog(parent = None, buttons = gtk.BUTTONS_OK,
                                           flags = gtk.DIALOG_MODAL, type = gtk.MESSAGE_ERROR,
                                           message_format = error )

            msgbox.set_title(Global.ERRORCONNECTIONTITLE)
            #result = msgbox.run()
            #msgbox.destroy()

