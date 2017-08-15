#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_DlgConnectionManager.py
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

from Global import Global
from FB_EIB import FB_EIBConnection


import pickle
import sqlite3



class FB_DlgConnectionManager(object):

    __GladeObj = None
    __cbConnections = None


    __txtName = None
    __cbType = None
    __txtListConnections = None    #widget of scrolltext to show all created connections
    __checkDefault = None

    __lblCOMPort = None        #widgets...
    __lblIP_Name = None
    __lblIP_IP = None
    __lblIP_Port = None
    __cbCOMPort = None
    __txtIP_Name = None
    __txtIP_IP = None
    __txtIP_Port = None

    _MyConnection = None    #current connection
    __connAtWork = False    #one connection to edit -> save settings to add a new one

    __curProject = None            #project object

    ##Constructor
    #@param curProject: object of the current project - ist only possible if a project has been loaded
    #@param cbConnections: widget combobox of the program-Frame to copy the data of the connection-list
    def __init__(self,curProject, parent):

        self.__curProject = curProject
        #self.__cbConnections = cbConnections
        self.__GladeObj = gtk.glade.XML(Global.GUIPath + Global.GladeFile,"dlgConnectionManager")
        #self.__dlgConnectionManager = self.__GladeObj.get_widget("dlgConnectionManager")

        dic = {"on_cbType_changed":self.ConnectionTypChanged ,
               "on_bNew_clicked":self.bNewClicked,
               "on_bDelete_clicked":self.bDeleteClicked,
               "on_txtListConnections_cursor_changed":self.ListViewConnectionsChanged,
               }
        self.__GladeObj.signal_autoconnect(dic)

        #get the widget of parameter frame
        self.__txtName = self.__GladeObj.get_widget("txtName")
        self.__txtListConnections = self.__GladeObj.get_widget("txtListConnections")
        self.__checkDefault = self.__GladeObj.get_widget("checkDefault")

        self.__lblCOMPort = self.__GladeObj.get_widget("lblCOMPort")
        self.__lblIP_Name = self.__GladeObj.get_widget("lblIP_Name")
        self.__lblIP_IP = self.__GladeObj.get_widget("lblIP_IP")
        self.__lblIP_Port = self.__GladeObj.get_widget("lblIP_Port")
        self.__cbCOMPort = self.__GladeObj.get_widget("cbCOMPort")
        self.__txtIP_Name = self.__GladeObj.get_widget("txtIP_Name")
        self.__txtIP_IP = self.__GladeObj.get_widget("txtIP_IP")
        self.__txtIP_Port = self.__GladeObj.get_widget("txtIP_Port")

        #load first entry of connectiontype as intialization
        #get widget of combo
        self.__cbType = self.__GladeObj.get_widget("cbType")
        self.__cbType.set_active(Global.ConTypes.Eibnet.index)    #first entry - init a eibnet
        self.SwitchFrameContent(Global.ConTypes.Eibnet.index)

        #load connections from Database...
        parent.LoadConnectionFromDB()

        self.InitConnectionView(0)

        dlgConnectionManager = self.__GladeObj.get_widget("dlgConnectionManager")
        #OK-Button
        if(dlgConnectionManager.run() == 1 and self.__curProject <> None and self._MyConnection <> None):

            #set the correct connection type
            self._MyConnection.setType(self.__cbType.get_active())
            #get Existing flag and Index in list
            Exist,Index = self.ConnectionExist(self._MyConnection)

            #reset all default-flags if current con is marked as default-con.
            if self.__checkDefault.get_active() == True:
                for i in range(len(self.__curProject.eibConnectionList)):
                    self.__curProject.eibConnectionList[i].setDefault(False)
            else:
                #default connection not checked -> check if at least one other conn. is checked
                #if not -> set current connection as default connection
                if self.CheckDefaultFlag() == False:
                    self.__checkDefault.set_active(True)

            if Exist == False:
                #add the new connection and update vars before inserting in list
                self._MyConnection.setName(self.__txtName.get_text())
                self._MyConnection.setType(self.__cbType.get_active())
                self._MyConnection.setCOM(self.__cbCOMPort.get_active())
                self._MyConnection.setIpName(self.__txtIP_Name.get_text())
                self._MyConnection.setIP(self.__txtIP_IP.get_text())
                self._MyConnection.setIPPort(self.__txtIP_Port.get_text())
                self._MyConnection.setDefault(self.__checkDefault.get_active())

                self.__curProject.eibConnectionList.append(self._MyConnection)

                self.SaveConnectionToDB(self._MyConnection,True)

            else:
                #connection exist -> update values in list
                self.__curProject.eibConnectionList[Index].setName(self.__txtName.get_text())
                self.__curProject.eibConnectionList[Index].setType(self.__cbType.get_active())
                self.__curProject.eibConnectionList[Index].setCOM(self.__cbCOMPort.get_active())
                self.__curProject.eibConnectionList[Index].setIpName(self.__txtIP_Name.get_text())
                self.__curProject.eibConnectionList[Index].setIP(self.__txtIP_IP.get_text())
                self.__curProject.eibConnectionList[Index].setIPPort(self.__txtIP_Port.get_text())
                self.__curProject.eibConnectionList[Index].setDefault(self.__checkDefault.get_active())

                self.SaveConnectionToDB(self.__curProject.eibConnectionList[Index],False)
            #self.__curProject.isChanged = True

        #call function to update the combobox in parentframe to show/select for user
        #self.UpdateUserConnections(self.__cbConnections)
        parent.UpdateUserConnections()

        self.__connAtWork = False

        dlgConnectionManager.destroy()

    def Run(self):
        pass



    ##Combobox - ConnectionType has been changed
    def ConnectionTypChanged(self,widget, Data=None):
        self.SwitchFrameContent(widget.get_active())

    ##selection of listbox Connections has been changed
    def ListViewConnectionsChanged(self,widget, Data=None):
        sel = widget.get_selection()
        (model, iter) = sel.get_selected()
        #get ID - column 2
        id = model.get_value(iter, 2)
        #look at the index within the connection-list with teh given ID
        for i in range(len(self.__curProject.eibConnectionList)):
            if self.__curProject.eibConnectionList[i].getID() == id:
                self.UpdateConnectionView(i)
                break

    ##Button: insert new Connection
    def bNewClicked(self,widget,Data=None):
        if  self.__connAtWork == False:
            self._MyConnection = FB_EIBConnection.FB_EIBConnection()        #create a new instance of a eib-connection
            #init widgets with init-connection-data
            self.__txtName.set_text(self._MyConnection.getName())

            image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "Connection.png")

            liststore = self.__txtListConnections.get_model()
            ListIterator = liststore.append([image,self._MyConnection.getName(),self._MyConnection.getID()])

            #set as default
            self.__checkDefault.set_active(True)
            #init
            self.__cbCOMPort.set_active(self._MyConnection.getCOM())
            self.__txtIP_Name.set_text(self._MyConnection.getIpName())
            self.__txtIP_IP.set_text(self._MyConnection.getIP())
            self.__txtIP_Port.set_text(str(self._MyConnection.getIPPort()))

        self.__connAtWork = True


    ##Button: delete selected Connection
    def bDeleteClicked(self,widget,Data=None):
        #get selected item
        #try:

            ListSelection =  self.__txtListConnections.get_selection()
            (model, iter) = ListSelection.get_selected()
            id = model.get_value(iter, 2)

            #get instance from list by id
            for i in range(len(self.__curProject.eibConnectionList)):
                    if id == self.__curProject.eibConnectionList[i].getID():
                        del self.__curProject.eibConnectionList[i]
                        model.remove(iter)
                        self.DeleteConnectionDB(id)

                        if len(self.__curProject.eibConnectionList) > 0:
                            self._MyConnection = self.__curProject.eibConnectionList[0]
                            self.UpdateConnectionView(0)
                        else:
                            #no more connections in list -> clear
                            self._MyConnection = None
                            self.UpdateConnectionView(-1)
                        break

        #except:
            #pass

    ##switches the frame content according to the current connection type
    #@param value: 0 = eibnet, 1 = serial FT1.2,...
    def SwitchFrameContent(self,value):

        #Eibnet
        if value == Global.ConTypes.Eibnet.index:
            self.__lblCOMPort.set_child_visible(False)
            self.__cbCOMPort.set_child_visible(False)

            self.__lblIP_Name.set_child_visible(True)
            self.__lblIP_IP.set_child_visible(True)
            self.__lblIP_Port.set_child_visible(True)
            self.__txtIP_Name.set_child_visible(True)
            self.__txtIP_IP.set_child_visible(True)
            self.__txtIP_Port.set_child_visible(True)
        #serial FT1.2
        elif value == Global.ConTypes.sFT12.index:

            self.__lblCOMPort.set_child_visible(True)
            self.__cbCOMPort.set_child_visible(True)

            self.__lblIP_Name.set_child_visible(False)
            self.__lblIP_IP.set_child_visible(False)
            self.__lblIP_Port.set_child_visible(False)
            self.__txtIP_Name.set_child_visible(False)
            self.__txtIP_IP.set_child_visible(False)
            self.__txtIP_Port.set_child_visible(False)

    ##initalize Connection-items
    #@param ConIndex: Index of Connection
    def InitConnectionView(self, ConIndex):

        #create ListStore object with 1xpicture and 1xstring entry (2nd string to store id - not shown)
        conListStore = gtk.ListStore(gtk.gdk.Pixbuf, str, str)
        self.__txtListConnections.set_model(conListStore)
        self.text_cell = gtk.CellRendererText()            #Text Object
        self.img_cell = gtk.CellRendererPixbuf()           #Image Object
        self.column = gtk.TreeViewColumn()
        self.column.pack_start(self.img_cell, False)
        self.column.pack_start(self.text_cell,True)
        self.column.add_attribute(self.img_cell, "pixbuf",0)
        self.column.add_attribute(self.text_cell, "text", 1)
        self.column.set_attributes(self.text_cell, markup=1)
        self.__txtListConnections.append_column(self.column)


        #load instances and put data in form
        if self.__curProject <> None:
            if len(self.__curProject.eibConnectionList) > 0:
                self.UpdateConnectionView(ConIndex)
                #show all connection in listview
                image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "Connection.png")

                for i in range(len(self.__curProject.eibConnectionList)):
                    ListIterator = conListStore.append([image,
                                                        self.__curProject.eibConnectionList[i].getName(),
                                                        self.__curProject.eibConnectionList[i].getID() ])

    def UpdateConnectionView(self, ConIndex):

        if ConIndex >= 0:
            #init local connectionobject
            self._MyConnection = self.__curProject.eibConnectionList[ConIndex]
            #write widgetvalues
            self.__txtName.set_text(self.__curProject.eibConnectionList[ConIndex].getName())
            self.__cbType.set_active(self.__curProject.eibConnectionList[ConIndex].getType())

            self.__checkDefault.set_active(self.__curProject.eibConnectionList[ConIndex].getDefault())
            self.__cbCOMPort.set_active(self.__curProject.eibConnectionList[ConIndex].getCOM())
            self.__txtIP_Name.set_text(self.__curProject.eibConnectionList[ConIndex].getIpName())
            self.__txtIP_IP.set_text(self.__curProject.eibConnectionList[ConIndex].getIP())
            self.__txtIP_Port.set_text(str(self.__curProject.eibConnectionList[ConIndex].getIPPort()))
        else:
            self.__txtName.set_text("")
            self.__checkDefault.set_active(False)
            self.__txtIP_Name.set_text("")
            self.__txtIP_IP.set_text("")
            self.__txtIP_Port.set_text("")

    ##check for existing connection
    #@param con: connection to check
    def ConnectionExist(self,con):
        RValue,Index = False,0

        if self.__curProject <> None:
            if self.__curProject.eibConnectionList <> None:
                for i in range(len(self.__curProject.eibConnectionList)):
                    if con.getID() == self.__curProject.eibConnectionList[i].getID():
                        RValue = True
                        Index = i
                        break
                    else:
                        RValue = False
                        Index = 0

        return RValue,Index

    ##check for at least one Default-Connection
    #@return: True = one Connection in list is already the default connection
    def CheckDefaultFlag(self):
        RValue = False
        for i in range(len(self.__curProject.eibConnectionList)):
            if (self.__curProject.eibConnectionList[i].getDefault() == True and
               self.__curProject.eibConnectionList[i].getID() <> self._MyConnection.getID()):
                RValue = True
                break

        return RValue



    def SaveConnectionToDB(self,MyConnection, New):

        cursor = Global.DatabaseConnection.cursor()

        #change Default-Flag in case new connection is the new default -connection
        if self.__checkDefault.get_active() == True:
            cursor.execute('SELECT * FROM Connections')
            rowList = cursor.fetchall()
            #row[0] = id
            #row[1] = Name
            #row[2] = Data
            for i in range(len(rowList)):
                #load instance
                tmpCon = pickle.loads(rowList[i][2])    #column 2 contains class data
                tmpCon.setDefault(False)        #reset flag to Defaultconnection
                #serialize back
                tmpser = pickle.dumps(tmpCon, 0)
                values = [sqlite3.Binary(tmpser)]
                cursor.execute('UPDATE Connections SET Data = ?  WHERE id = ' + str(rowList[i][0]), values)
            Global.DatabaseConnection.commit()

        cursor = Global.DatabaseConnection.cursor()
        #serialize MyConnection
        ser = pickle.dumps(MyConnection, 0)

        if New == True:
            values = [MyConnection.getName(), sqlite3.Binary(ser) ]
            cursor.execute('INSERT INTO Connections VALUES (null,?,?)', values)
            Global.DatabaseConnection.commit()

        else:
            #update list
            #find connection in list of database
            cursor.execute('SELECT * FROM Connections')
            #row[0] = id
            #row[1] = Name
            #row[2] = Data
            for row in cursor:
                tmpCon = pickle.loads(row[2])    #column 2 contains class data
                if tmpCon.getID() == MyConnection.getID():
                    ser = pickle.dumps(MyConnection, 0)
                    values = [MyConnection.getName(), sqlite3.Binary(ser) ]
                    cursor.execute('UPDATE Connections SET Name = ?, Data = ?  WHERE id = ' + str(row[0]), values)
                    Global.DatabaseConnection.commit()
                    break

    def DeleteConnectionDB(self,id):
        #delete entire table in DB- after deleting, each connection will be restored, besides the deleted connection
        cursor = Global.DatabaseConnection.cursor()
        cursor.execute('SELECT * FROM Connections')
        for row in cursor:
            tmpCon = pickle.loads(row[2])    #column 2 contains class data
            if tmpCon.getID() == id:
                dbID = row[0]
                cursor.execute('DELETE FROM Connections WHERE id = ' + str(dbID))
                Global.DatabaseConnection.commit()
                break

