#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_MainFrame.py
#Version: V0.1 , 09.02.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
import sys
import time
import thread
import Queue
import threading
import thread
import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade

#from configobj import ConfigObj
from LOG import Logging
from Global import Global

from GUI import FB_NewProjectWindow
from GUI import FB_OpenProjectWindow
from GUI import FB_DlgDeviceData
from GUI import FB_ProgramFrame
from GUI.Tree import FB_ArchitecturalTree
from GUI.Tree import FB_TopologyTree
from GUI.Tree import FB_GroupAdressTree
from DATABASE import FB_DlgDatabase

#import FB_PROJECT
#import src.XML
from FB_PROJECT import FB_ArchitecturalDataModel
from FB_PROJECT import FB_AdressLogic
from FB_PROJECT import configobj
from XML import FB_XMLConverter
from XML import FB_XML_PRODUCT

import jpype
#import socket

import sqlite3

class MainFrame(object):

    __WindowWidth = 0
    __WindowHeigth = 0
    __LogObj = None
    __CurProjectObj = None         #Project object
    __CurArchObj = None            #architectural model of current project
    __CurInstObj = None            #installation model of the current project
    __AdressLogicObj = None        #object to handle the entire adress logic of the CurProjectObj
    __GladeObj = None

    __GUIPath = ""
    __ImagePath = ""
    __ProjTree = None
    __TreeIterator = None        #Iterator-ObAject for Tree
    __ArchTree = None            #Object for Tree-Class
    __handelboxToolbar = None    #widget object of handlebox toolbar
    __mnuPopup = None            #Object for popup menu
    __TopologyTree = None        #widget of topology treeview
    __GroupAdressTree = None     #widget of group adress treeview
    __PopUpGlade = None

    __popupNewHandlerID = 0      #signale Hander ID for popupMenu "New Item" button
    __popupEditHandlerID = 0     #signal handler ID for popupMenu "Change Name" button

    #object for Communications objects handling

    __commObjList = None         #widget of Communication Object List
    __commObjListStore = None    #ListModel interface


    #--- menu buttons ----
    __ShowProgramWindow = None

    #statusbar vars
    __MainStatusBar = None
    __contextID = None        #generall context id belong to the statusbar
    __messageIDDatabase = None #meesage id for information Database connection


    treestore = None
    curDragDataType = 0            #the current Type of draged data (builidng,floor,rooms---)

    tmpCounter = 0
    __lastValidAdress = 0

    def __init__(self):
    # create a new window
        LogFileName = Global.LogPath + 'MainFrame.log'
        Options = 0
        self.__LogObj = Logging.Logging("FB_MainFrame",LogFileName,Options)

        self.__CurProjectObj = None
        self.__CurInstObj = None

        #get screensize
        self.__WindowWidth = gtk.gdk.screen_width()
        self.__WindowHeigth = gtk.gdk.screen_height()

        self.__GladeObj = gtk.glade.XML(Global.GUIPath  + Global.GladeFile,"MainFrame")
        #get widget of window
        self.window = self.__GladeObj.get_widget("MainFrame")
        self.__MainStatusBar = self.__GladeObj.get_widget("MainStatusBar")
        self.__contextID = self.__MainStatusBar.get_context_id("Statusbar in FB_MainFrame")

        self.__contextID_DB = self.__MainStatusBar.get_context_id("Test")
        self.__messageIDDatabase = self.__MainStatusBar.push(self.__contextID, "Database: ")

        #self.__messageIDDatabase = self.__MainStatusBar.push(self.__contextID, "Database: doof ")
        #self.__messageIDDatabase = self.__MainStatusBar.push(self.__contextID_DB, "Database: hallo ")

        #FT12Connection = jpype.JClass("tuwien.auto.calimero.serial.FT12Connection")
        #z = FT12Connection(8)
        #print z.getState()

        #self.__LogObj.NewLog("doof",1)
        #Databaseconnection
        config = configobj.ConfigObj(Global.settingFile)
        self.__LogObj.NewLog(Global.settingFile,1)

        try:
            Global.DatabaseConnection = sqlite3.connect(config['Database']['Database'])
            Global.DatabaseConnection.text_factory = str
        except:
            Global.DatabaseConnection = None
            pass



        #probably will never be None -> sqlite creats a database if its not connectable...
        if(Global.DatabaseConnection == None):
            msgbox = gtk.MessageDialog(parent = self.window, buttons = gtk.BUTTONS_OK,
                                           flags = gtk.DIALOG_MODAL, type = gtk.MESSAGE_WARNING,
                                           message_format = Global.ERROROPENDATABASE )

            msgbox.set_title(Global.ERROROPENDATABASETITLE)
            result = msgbox.run()
            msgbox.destroy()



        if(self.__GladeObj == None):
           self.__LogObj.NewLog("Error at intializing GUI-Interface (Glade-Object-MainFrame)",1)
           return

        #-------------------------------------------------------------------------------------

        #popupmenu init
        self.__PopUpGlade = gtk.glade.XML(Global.GUIPath  + Global.GladeFile,"mnuPopupTree")
        self.__mnuPopup =  self.__PopUpGlade.get_widget("mnuPopupTree")

        if(self.__mnuPopup == None):
           self.__LogObj.NewLog("Error at intializing GUI-Interface (Glade-Object-Popup Menu)",1)
           return

        pop = {
                #Popupmenu
               # "on_mnuNewItem_activate": self.PopupNew,
                "on_mnuChangeName_activate": self.PopupChangeName,
                "on_mnuDeleteGroup_activate": self.PopupDelete,
                "on_mnuPropertyGroup_activate":self.PopupProperty

                }

        self.__PopUpGlade.signal_autoconnect(pop)


       #-------------------------------------------------------------------------------------

       #popupmenu Topology

       #-------------------------------------------------------------------------------------

        #setting up DragNDrop of (Source) Toolbarbuttons
        #1. get widget of building button
        self.widget_building = self.__GladeObj.get_widget("button_building")
        self.widget_building.drag_source_set(gtk.gdk.BUTTON1_MASK,[('text/plain', gtk.TARGET_SAME_APP, Global.DND_BUILDING)],gtk.gdk.ACTION_COPY)
        #2. get widget of floor button
        self.widget_floor = self.__GladeObj.get_widget("button_floor")
        self.widget_floor.drag_source_set(gtk.gdk.BUTTON1_MASK,[('text/plain', gtk.TARGET_SAME_APP, Global.DND_FLOOR)],gtk.gdk.ACTION_COPY)
        #3. get widget of room button
        self.widget_room = self.__GladeObj.get_widget("button_room")
        self.widget_room.drag_source_set(gtk.gdk.BUTTON1_MASK,[('text/plain', gtk.TARGET_SAME_APP, Global.DND_ROOM)],gtk.gdk.ACTION_COPY)
        #4. get widget of junctionbox button
        self.widget_junction = self.__GladeObj.get_widget("button_junction")
        self.widget_junction.drag_source_set(gtk.gdk.BUTTON1_MASK,[('text/plain', gtk.TARGET_SAME_APP, Global.DND_JUNCTION)],gtk.gdk.ACTION_COPY)



        #get widget of handlebox_toolbar
        self.__handelboxToolbar = self.__GladeObj.get_widget("handlebox_toolbar")
        #hide control at project start
        self.__handelboxToolbar.hide()

        #get widget object of PanelLeft (Project-Tree)
        self.Tree = self.__GladeObj.get_widget("ProjectTree")
        self.__ProjTree = self.Tree

        #get widget object of Topology Tree
        self.__TopologyTreeWidget = self.__GladeObj.get_widget("TopologyTree")
        self.__commObjList = self.__GladeObj.get_widget("CommObjList")

        #get widget object of GroupAdress Tree
        self.__GroupAdressTreeWidget = self.__GladeObj.get_widget("GroupAddressTree")

        dic = { "on_MainFrame_destroy" : self.QuittApp ,
                "on_Quitt_activate" : self.QuittApp,
                #Menu items
                "on_new_project_activate" : self.MenuNewProject,
                "on_open_project_activate" : self.MenuOpenProject,
                "on_Save_activate" : self.MenuSaveProject,
                "on_button_drag_data_get" : self.DragDataGet,
                "on_ConvertDeviceData_activate":self.Converter,
                "on_ImportDeviceData_activate":self.ImportProductData,
                "on_ShowDeviceData_activate":self.ShowDeviceData,
                "on_DlgDatabase_activate":self.DatabaseSetting,

                #Menu: Program
                "on_Program_activate":self.ShowProgramWindow,



                "on_About_activate":self.ShowAboutDlg,

                #ProjectTree
                "on_ProjectTree_drag_data_received" : self.ProjectTreeDropData,
                "on_ProjectTree_drag_motion" : self.ProjectTreeDragMotion,
                "on_ProjectTree_button_press_event": self.ProjectTreeButtonPress,

                #TopologyTree
                "on_TopologyTree_button_release_event":self.TopologyTreeButtonPress,
                "on_TopologyTree_cursor_changed":self.TopologySelectionChanged,

                #GroupAddressTree
                "on_GroupAddressTree_button_release_event":self.GroupAdressTreeButtonPress,



                }
        self.__GladeObj.signal_autoconnect(dic)

        #-------------------------------------------------------------------------------------
        self.__ShowProgramWindow = self.__GladeObj.get_widget("Program")
        self.__ShowProgramWindow.set_state(gtk.STATE_INSENSITIVE)

        #create Project Tree
        self.__ArchTree = FB_ArchitecturalTree.FB_ArchitecturalTree(self.__LogObj,self.__ProjTree)
        self.__TopologyTree = FB_TopologyTree.FB_TopologyTree(self.__LogObj,self.__TopologyTreeWidget)
        self.__GroupAdressTree = FB_GroupAdressTree.FB_GroupAdressTree(self.__LogObj,self.__GroupAdressTreeWidget)


    #create a new project
    def MenuNewProject(self,widget, data=None):
        newProjectWin = FB_NewProjectWindow.FB_NEWPROJECTWINDOW(self.__LogObj,self)

    #open an existing project
    def MenuOpenProject(self,widget, data=None):
        openProjectWin = FB_OpenProjectWindow.FB_OPENPROJECTWINDOW(self.__LogObj, self)

    #save project data
    def MenuSaveProject(self,widget, data=None):
        if(self.__CurProjectObj <> None):

            self.__CurProjectObj.SaveProject()
        else:
            #print "Fehler save "
            self.__LogObj.NewLog("Error at saving Projectdata -> CurProjectObj is Nonetype",1)

    #show Database-settings-Dialog
    def DatabaseSetting(self, widget, data=None):
        FB_DlgDatabase.FB_DlgDatabase(self.__LogObj)

    #show dlg device data
    def ShowDeviceData(self,widget, data=None):
        FB_DlgDeviceData.FB_DlgDeviceData(self.__LogObj,self.__AdressLogicObj,True)
    #start the converter dialog
    def Converter(self,widget, data=None):
        XML = FB_XMLConverter.FB_XMLConverter(self.__LogObj)

    def ConverterThread(self, XMLConverter):
        XMLConverter()

    #start Import to SQL Dialog
    def ImportProductData(self,widget, data=None):
        FBProductData = FB_XML_PRODUCT.FB_XML_PRODUCT(self.__LogObj)

    #Menu: Program
    def ShowProgramWindow(self,widget, data=None):
        FB_ProgramFrame.FB_ProgramFrame(self.__CurProjectObj)


    #set current project object
    def SetCurrProject(self, ProjObj):
        self.__CurProjectObj = ProjObj
        self.__CurArchObj = self.__CurProjectObj.getArchModel()
        self.__CurInstObj = self.__CurProjectObj.getInstModel()


        if self.__CurProjectObj <> None:
            self.__ShowProgramWindow.set_sensitive(True)

        #create instance of adresslogic
        self.__AdressLogicObj = FB_AdressLogic.FB_AdressLogic(self.__LogObj,ProjObj)
        self.__CurProjectObj.setAdressLogicObj(self.__AdressLogicObj)  #save the global adresslogic instance

        #reorganize our project-tree
        if(self.__ProjTree <> None):
            self.__ArchTree.ClearTree()
            self.__ArchTree.CreateNewTree(self.__CurProjectObj)

            self.__TopologyTree.ClearTree()
            self.__TopologyTree.CreateNewTree(self.__CurProjectObj)

            self.__GroupAdressTree.ClearTree()
            self.__GroupAdressTree.CreateNewTree(self.__CurProjectObj)

            self.__handelboxToolbar.show()


    #get the type of the draged data
    def DragDataGet(self,widget, drag_context, selection, targetID, time):
        self.curDragDataType = targetID

    #add object to tree -> drop object over treeview
    def ProjectTreeDropData(self,treeview, drag_context, x, y, selection, targetID, timestamp):
        drop_info = treeview.get_dest_row_at_pos(x, y)
        if drop_info:
            model = treeview.get_model()
            path, position = drop_info

            #compare info with position/path object and add building,florr,room ...
            #get ID of selected item in treeview (iter path)
            Iterator = model.get_iter(path)
            #position is always 2 (column) to idendifiy the type of node
            #parent-element (ex: project-1,building-1 ...
            ParentID = model.get_value(Iterator, 2)
            #selected ID (ex:project, building,room, floor...
            SelID = self.__ArchTree.ArchModel.getPrefix(self.curDragDataType-1)

            #remove last part (-1 ..)
            end = ParentID.find('-')
            if(end <> -1):
                Type = ParentID[0: end]
            else:
                Type = ParentID

            #wenn value = project, dann darf nur Typ building eingefügt werden usw...
            #thats why -> targetID - 1 (building -> project; floor->building ...)
            if (SelID == Type):
                PicturePrefix = self.__ArchTree.ArchModel.getPrefix(self.curDragDataType)
                self.OpenDlgNewStructureElement(ParentID, Iterator,self.__ArchTree, PicturePrefix)


    #check if type ok for drop position (cursorlayout!)
    #currently not completed --- for later use ti optimize
    def ProjectTreeDragMotion(self,treeview, drag_context, x, y, time):
        drop_info = treeview.get_dest_row_at_pos(x, y)
        if drop_info:
            model = treeview.get_model()
            path, position = drop_info
            #compare info with position/path object and add building,florr,room ...
            #get ID of selected item in treeview (iter path)
            Iterator = model.get_iter(path)
            #position is always 2 (column) to idendifiy the type of node
            value = model.get_value(Iterator, 2)

            #wenn value = project, dann darf nur Typ building eingefügt werden usw...
            #thats why -> targetID - 1 (building -> project; floor->building ...)
            if (value == self.__ArchTree.ArchModel.getPrefix(self.curDragDataType-1)):
                pass

    #handles the general dialog to add any structure element
    def OpenDlgNewStructureElement(self, ParentID, TreeIterator, TreeObject, PicturePrefix):
        #open dialog to give a new name
        #AddStructureElement = FB_AddStructureElement.FB_AddStructureElement(self.__LogObj, self,"Reserve")
        GladeObj = gtk.glade.XML(Global.GUIPath + Global.GladeFile,"DlgAddStructureElement")

        lblAdress = GladeObj.get_widget("lblAdress")
        spinAdress = GladeObj.get_widget("spinAdress")

        window = GladeObj.get_widget("DlgAddStructureElement")
        #create title-text depending on added element
        (title, AdressFieldVisible, lblAdressText, spinMax) = self.__ArchTree.ArchModel.getTypeStructureElement(PicturePrefix)
        window.set_title(title)

        #changed value at spinadress -> DlgStructureElement
        localHandler = spinAdress.connect("value_changed",self.spinAdressValueChanged, (PicturePrefix,ParentID))

        if(AdressFieldVisible == True):
            lblAdress.show()
            spinAdress.show()
            lblAdress.set_text(lblAdressText)
            spinAdress.set_range(0,spinMax)
            #first call the AdressCheckRoutine
            spinAdress.set_value(self.__AdressLogicObj.GetNextPossibleAdress(PicturePrefix,0, True,ParentID))

        else:
            lblAdress.hide()
            spinAdress.hide()


        txtElementName = GladeObj.get_widget("txtElementName")

        #wait for closing dialog
        response = window.run()

        if(response == gtk.RESPONSE_OK):
        #Default Name
            Name = txtElementName.get_text()

            #add object and gets the new ID (topology-1 ...)
            ID = self.__CurArchObj.addChild(ParentID)

            if(Name == ""):
                Name = ID

            if(ID <> None):
                self.__CurArchObj.setName(ID,Name)
                if(self.__CurArchObj.getNodeName(ID) == self.__CurArchObj.TOPOLOGY_AREA or
                   self.__CurArchObj.getNodeName(ID) == self.__CurArchObj.TOPOLOGY_LINE or
                   self.__CurArchObj.getNodeName(ID) == self.__CurArchObj.GROUPADRESS_MAIN or
                   self.__CurArchObj.getNodeName(ID) == self.__CurArchObj.GROUPADRESS_MIDDLE or
                   self.__CurArchObj.getNodeName(ID) == self.__CurArchObj.GROUPADRESS ):

                    self.__CurArchObj.setAdress(ID, int(spinAdress.get_value()))
                    #recreate Name with adress comment
                    name = self.__CurArchObj.getName(ID)
                    self.__CurArchObj.setName(ID,name + " (" + str(int(spinAdress.get_value())) + ")")

                TreeObject.CreateTreeNode(ID, TreeIterator, PicturePrefix)

            spinAdress.disconnect(localHandler)
            window.destroy()
        else:
            spinAdress.disconnect(localHandler)
            window.destroy()

    #spinbutton value changed event of DlgStructureElement
    #data = PicturePrefix to iditify the element type (building,floor,..., area, line ...)
    def spinAdressValueChanged(self,widget, data):
        try:
            #data[0] = PicturePrefix
            #data[1] = ParentID

            newAdress = int(widget.get_value())
            #check if new Adress is valid and still possible to set
            AdressOK = self.__AdressLogicObj.CheckAdress(newAdress,data[0],data[1])

            if(AdressOK == True):
                self.__lastValidAdress = newAdress
            else:
                #set the next possible adress
                if(newAdress > self.__lastValidAdress):
                    Direction = True
                else:
                    Direction = False

                self.__lastValidAdress = self.__AdressLogicObj.GetNextPossibleAdress(data[0],newAdress, Direction,data[1])
                widget.set_value(self.__lastValidAdress)

        except:
            pass


    #callback of detach menu to popup
    def PopupDetach(self,menu, widget):
        pass
        #print "Detach"

    #general event handler to handle button events
    def ProjectTreeButtonPress(self,widget, event):
        #attach treeview "ProjectTree" to popup
        if(self.__mnuPopup.get_attach_widget() <> None):
            self.__mnuPopup.detach()
        self.__mnuPopup.attach_to_widget(widget,self.PopupDetach)
        #Ignore double-clicks and triple-clicks  -> do right click to show popup menu
        if (event.button == 3 and event.type == gtk.gdk.BUTTON_PRESS and self.__CurProjectObj <> None):
            #get New-Button Widget and unvisible it, only used in topology/group address tree

            NewButtonWidget = self.__PopUpGlade.get_widget("mnuNewItem")
            NewButtonWidget.hide()
            self.__mnuPopup.popup(None,None,None,event.button,event.time)

        #signal handler mouse button right click release at Treeview Topology
    def TopologyTreeButtonPress(self,widget,event):
        #attach treeview "TopologyTree" to popup
        if(self.__mnuPopup.get_attach_widget() <> None):
            self.__mnuPopup.detach()
        self.__mnuPopup.attach_to_widget(widget,self.PopupDetach)

        #Ignore double-clicks and triple-clicks  -> do right click to show popup menu
        if (event.button ==  3 and event.type == gtk.gdk.BUTTON_RELEASE and self.__CurProjectObj <> None ):
            #init Datafield
            Data = [0,0,0,0]

            #check which item has been clicked (area, line)
            #1. get treeview
            treeselection = self.__TopologyTreeWidget.get_selection()
            #Iterator = model.get_iter(path)
            (model, iter) = treeselection.get_selected()
            #get the widget object of the menuitem "new"
            NewButtonWidget = self.__PopUpGlade.get_widget("mnuNewItem")
            NewButtonWidget.hide()
            #in case of root element -> iter will be None-Type
            #check for root-selection
            #print model.iter_parent(iter)

            if(model.iter_parent(iter) == None):
                ParentID = self.__CurArchObj.TOPOLOGY_ROOT_ID

               # PicturePrefix = 20    #to idendiy the right picture for tree visualisation
                Data[0] = 1
                Data[1] = ParentID
                Data[2] = iter
                Data[3] = self.__CurArchObj.TOPOLOGY_AREA#PicturePrefix
                #NewButtonWidget.connect("activate",self.PopupNew,(1,ParentID,iter,PicturePrefix))
                label = NewButtonWidget.child
                label.set_text(unicode("Neuen Bereich anlegen"))
                self.ActivatePopupNewItem(Data,NewButtonWidget,event)

            #sub node selected (lines are possible)
            else:
                #get the ID (=ParentID) of the selected object -> thats the parent for our new line
                #we need the child ID (ID of the seletced object)

                ParentID =  model.get_value(iter,2) #get value of Column 2 = Attributevalue = ID (Column 0 = picture, 1 = Name)
                #check prefix (in case of line is selected -> only device can be added)
                #get the nodename and compare it with the prefix to know if line or area or whatever was selected
                ParentNodeName = self.__CurArchObj.getNodeName(ParentID)

                if(ParentNodeName == self.__CurArchObj.TOPOLOGY_AREA):
                    #PicturePrefix = 21
                    Data[0] = 2
                    Data[1] = ParentID
                    Data[2] = iter
                    Data[3] = self.__CurArchObj.TOPOLOGY_LINE #PicturePrefix
                    #NewButtonWidget.connect("activate",self.PopupNew,(2,ParentID,iter,PicturePrefix))
                    label = NewButtonWidget.child
                    label.set_text(unicode("Neue Linie anlegen"))
                    self.ActivatePopupNewItem(Data,NewButtonWidget,event)

                elif(ParentNodeName == self.__CurArchObj.TOPOLOGY_LINE):
                    #PicturePrefix = 22
                    Data[0] = 3
                    Data[1] = ParentID
                    Data[2] = iter
                    Data[3] = self.__CurArchObj.TOPOLOGY_DEVICE#PicturePrefix
                    #NewButtonWidget.connect("activate",self.PopupNew,(3,ParentID,iter,PicturePrefix))
                    label = NewButtonWidget.child
                    label.set_text(unicode("Neues Gerät einfügen","ISO-8859-1"))
                    self.ActivatePopupNewItem(Data,NewButtonWidget,event)

            self.__mnuPopup.popup(None,None,None,event.button,event.time)

    #signal handler mouse button right click release at Treeview GroupAdress
    def GroupAdressTreeButtonPress(self,widget,event):
        #attach treeview "TopologyTree" to popup
        if(self.__mnuPopup.get_attach_widget() <> None):
            self.__mnuPopup.detach()
        self.__mnuPopup.attach_to_widget(widget,self.PopupDetach)

        #Ignore double-clicks and triple-clicks  -> do right click to show popup menu
        if (event.button ==  3 and event.type == gtk.gdk.BUTTON_RELEASE and self.__CurProjectObj <> None ):
            #init Datafield
            Data = [0,0,0,0]

            #check which item has been clicked (area, line)
            #1. get treeview
            treeselection = self.__GroupAdressTreeWidget.get_selection()
            #Iterator = model.get_iter(path)
            (model, iter) = treeselection.get_selected()
            #get the widget object of the menuitem "new"
            NewButtonWidget = self.__PopUpGlade.get_widget("mnuNewItem")
            NewButtonWidget.hide()


            if(model.iter_parent(iter) == None):
                ParentID = self.__CurArchObj.GROUPADRESS_ROOT_ID

                #PicturePrefix = 30    #to idendiy the right picture for tree visualisation
                Data[0] = 4    #new maingroup
                Data[1] = ParentID
                Data[2] = iter
                Data[3] = self.__CurArchObj.GROUPADRESS_MAIN#PicturePrefix
                label = NewButtonWidget.child
                label.set_text(unicode("Neue Hauptgruppe anlegen"))
                self.ActivatePopupNewItem(Data,NewButtonWidget,event)

            else:
                ParentID =  model.get_value(iter,2) #get value of Column 2 = Attributevalue = ID (Column 0 = picture, 1 = Name)
                ParentNodeName = self.__CurArchObj.getNodeName(ParentID)

                #in case maingroup node is selected -> allow to create middle group
                if(ParentNodeName == self.__CurArchObj.GROUPADRESS_MAIN):
                    #PicturePrefix = 31
                    Data[0] = 5  #middle group
                    Data[1] = ParentID
                    Data[2] = iter
                    Data[3] = self.__CurArchObj.GROUPADRESS_MIDDLE#PicturePrefix
                    #NewButtonWidget.connect("activate",self.PopupNew,(2,ParentID,iter,PicturePrefix))
                    label = NewButtonWidget.child
                    label.set_text(unicode("Neue Mittelgruppe anlegen"))
                    self.ActivatePopupNewItem(Data,NewButtonWidget,event)

                #in case middlegroup node is selected -> allow to create subgroup
                elif(ParentNodeName == self.__CurArchObj.GROUPADRESS_MIDDLE):
                    #PicturePrefix = 32
                    Data[0] = 6  #subgroup
                    Data[1] = ParentID
                    Data[2] = iter
                    Data[3] = self.__CurArchObj.GROUPADRESS#PicturePrefix
                    #NewButtonWidget.connect("activate",self.PopupNew,(3,ParentID,iter,PicturePrefix))
                    label = NewButtonWidget.child
                    label.set_text(unicode("Neue Gruppenadresse einfügen","ISO-8859-1"))
                    self.ActivatePopupNewItem(Data,NewButtonWidget,event)


            self.__mnuPopup.popup(None,None,None,event.button,event.time)

    #---------------------------------------------------------------------------------------------------------------------------
    #----------------------------------------- Handling Popup Menu -------------------------------------------------------------
    #---------------------------------------------------------------------------------------------------------------------------
    def ActivatePopupNewItem(self,Data,NewButtonWidget,event):
        #in case the popup menu has been acitvated and now finished (canceled) we have to disconnect the event
        if(self.__popupNewHandlerID <> 0):
            NewButtonWidget.disconnect(self.__popupNewHandlerID)

        #Data[1] = ParentID (ex. AreaTopology-1)
        #Data[2] = iterator of tree
        #Data[3]= PicturePrefix (Identifier)
        if(self.__AdressLogicObj.MoreAdressPossible(Data[3],Data[1]) == True):
            NewButtonWidget.show()
        self.__popupNewHandlerID = NewButtonWidget.connect("activate",self.PopupNew,(Data[0],Data[1],Data[2],Data[3]))


    #popup menu -> New item
    def PopupNew(self,widget, data):
        #date structure:
            #[0] = type (1 = new Area of topology ; 2 ) new line in area of topology
            #[1] = Parent-ID of DOM-Node (in case of new area -> parent = topology-1,
            #[2] = iterator of tree
            #[3] = PicturePrefix for tree (building, floor, area, line ...)=

        type = data[0]
        ParentID = data[1]
        iter = data[2]
        PicPrefix = data[3]

        #-------------------------------------------------------------------------------------------------------------------------
        #------------------------------------------- Topology Adress section -----------------------------------------------------
        #-------------------------------------------------------------------------------------------------------------------------

        if(type == 1 or type == 2):
            #new area should be created or  #new line should be created
            self.OpenDlgNewStructureElement(ParentID,iter, self.__TopologyTree, PicPrefix)


        #-------------------------------------------------------------------------------------------------------------------------
        #------------------------------------------- Device Adress section -------------------------------------------------------
        #-------------------------------------------------------------------------------------------------------------------------
        #new device should be created
        elif(type == 3):
            #open the device-dialog with parameter show-only = false -> with adress fields
            #
            instance = FB_DlgDeviceData.FB_DlgDeviceData(self.__LogObj,self.__AdressLogicObj,False)
            instance.SetParentID(ParentID)
            instance.SetTopologyProperty(self.__TopologyTree,iter)
            instance.SetProjectObj(self.__CurProjectObj)

        #-------------------------------------------------------------------------------------------------------------------------
        #------------------------------------------- Group Adress section --------------------------------------------------------
        #-------------------------------------------------------------------------------------------------------------------------

        elif(type == 4 or type == 5 or type == 6):
            self.OpenDlgNewStructureElement(ParentID,iter, self.__GroupAdressTree, PicPrefix)
            #important to disconnect this event!
        else:
            pass

        #important to disconnect this event!
        #widget.disconnect(self.__popupNewHandlerID)
        widget.disconnect(self.__popupNewHandlerID)
        self.__popupNewHandlerID = 0

    #Popup activity ;
    #edit item name
    def PopupChangeName(self,widget, data=None):
       # try:
            menuParent = widget.parent.get_attach_widget()
            self.__mnuPopup.detach()

            #depending on widget which is attached to the popup do different things...
            #in case of "ProjectTree"
            if(menuParent.get_name() == self.__ProjTree.get_name()):
                #first get the right widget which was initiator
                #change to editmode
                self.__ArchTree.text_cell.set_property('editable', True)
                #find current selected position at treeview
                path,column = self.__ProjTree.get_cursor()
                self.__ProjTree.set_cursor( path,column ,True )
                model = self.__ProjTree.get_model()
                #conect callback
                self.__ArchTree.text_cell.connect('edited', self.CellChanged, model)

            elif(menuParent.get_name() == self.__TopologyTreeWidget.get_name()):
                #first get the right widget which was initiator
                #change to editmode
                self.__TopologyTree.text_cell.set_property('editable', True)
                #find current selected position at treeview
                path,column = self.__TopologyTreeWidget.get_cursor()
                self.__TopologyTreeWidget.set_cursor( path,column ,True )
                model = self.__TopologyTreeWidget.get_model()
                #conect callback
                self.__TopologyTree.text_cell.connect('edited', self.CellChanged, model)

            elif(menuParent.get_name() == self.__GroupAdressTreeWidget.get_name()):
                #first get the right widget which was initiator
                #change to editmode
                self.__GroupAdressTree.text_cell.set_property('editable', True)
                #find current selected position at treeview
                path,column = self.__GroupAdressTreeWidget.get_cursor()
                self.__GroupAdressTreeWidget.set_cursor( path,column ,True )
                model = self.__GroupAdressTreeWidget.get_model()
                #conect callback
                self.__GroupAdressTree.text_cell.connect('edited', self.CellChanged, model)

      #  except:
       #     pass

    #will be called if a item has been changed (name of node...)
    def CellChanged(self,cell, path, new_text, user_data):
        #get treeview object from context (user_data)
        treeview = user_data
        #write new text to tree at column 1
        treeview[path][1] = new_text
        #get Iterator of given path (selected item)
        Iterator = treeview.get_iter(path)
        #find ID from cloumn 2 (building-2 etc.)
        ID = treeview.get_value(Iterator, 2)
        #write new text to architectural model (internal data)
        #ArchTree is OK; we dont nedd the TopologyTree here, because internal the ArchModel is the one and only
        #in the project (belongs in every xxTree-module)
        self.__ArchTree.ArchModel.setName(ID,new_text)
        #reset editable property of tree
        self.__ArchTree.text_cell.set_property('editable', False)
        self.__TopologyTree.text_cell.set_property('editable', False)
        self.__GroupAdressTree.text_cell.set_property('editable', False)

        self.__CurProjectObj.isChanged = True


    #popup acitivity -> delete item
    def PopupDelete(self,widget, data=None):

        try:
            menuParent = widget.parent.get_attach_widget()
            self.__mnuPopup.detach()

            #depending on widget which is attached to the popup do different things...
            #in case of "ProjectTree"
            if(menuParent.get_name() == self.__ProjTree.get_name()):
                #get the selected item
                treeselection = self.__ProjTree.get_selection()
                treestore, Iterator = treeselection.get_selected()
                #get ID of selected item
                Attr = treestore.get_value(Iterator, 2)

                end = Attr.find('-')
                if(end <> -1):
                    Type = Attr[0: end]
                else:
                    Type = Attr

                if(Type <> self.__ArchTree.ArchModel.getPrefix(Global.DND_PROJECT)):
                    #finaly:
                    #delete treeobjects
                    treestore.remove(Iterator)
                    #delete internal data
                    self.__ArchTree.ArchModel.removeData(Attr)
                    self.__CurProjectObj.isChanged = True
            #Topology Tree
            elif(menuParent.get_name() == self.__TopologyTreeWidget.get_name()):
                #get the selected item
                treeselection = self.__TopologyTreeWidget.get_selection()
                treestore, Iterator = treeselection.get_selected()
                #get ID of selected item
                Attr = treestore.get_value(Iterator, 2)
                end = Attr.find('-')
                if(end <> -1):
                    Type = Attr[0: end]
                else:
                    Type = Attr

                if(Type <> self.__ArchTree.ArchModel.getPrefix(Global.DND_PROJECT)):
                    #finaly:
                    #delete treeobjects
                    treestore.remove(Iterator)
                    #delete internal data
                    self.__CurArchObj.removeData(Attr)
                    self.__CurInstObj.removeData(Attr)
                    self.__CurProjectObj.isChanged = True

            #Groupadress Tree
            elif(menuParent.get_name() == self.__GroupAdressTreeWidget.get_name()):
                #get the selected item
                treeselection = self.__GroupAdressTreeWidget.get_selection()
                treestore, Iterator = treeselection.get_selected()
                #get ID of selected item
                Attr = treestore.get_value(Iterator, 2)
                end = Attr.find('-')
                if(end <> -1):
                    Type = Attr[0: end]
                else:
                    Type = Attr

                if(Type <> self.__ArchTree.ArchModel.getPrefix(Global.DND_PROJECT)):
                    #finaly:
                    #delete treeobjects
                    treestore.remove(Iterator)
                    #delete internal data
                    self.__GroupAdressTree.ArchModel.removeData(Attr)
                    self.__CurProjectObj.isChanged = True

        except:
            pass

    #popup activity -> show propertys
    def PopupProperty(self,widget, data=None):
        print "Prop"


    #event: Selection of Topology-Tree has been changed
    def TopologySelectionChanged(self, widget, data=None):
        #widget:gtkTreeView
        #get data of selected row
        treeselection = widget.get_selection()
        treestore, Iterator = treeselection.get_selected()
        #get the column value (2) -> column2 = ID
        ID =  treestore.get_value(Iterator,2)
        #get the information from the installationfile


    ##menu item "About"
    def ShowAboutDlg(self,widget, data=None):
        #get the dlgAbout widget
        tmpGlade = gtk.glade.XML(Global.GUIPath  + Global.GladeFile,"DlgAbout")
        #get widget of window
        DlgAbout = tmpGlade.get_widget("DlgAbout")
        DlgAbout.run()
        DlgAbout.destroy()


    def main(self):
    # All PyGTK applications must have a gtk.main(). Control ends here
    # and waits for an event to occur (like a key press or mouse event).
      #  gtk.gdk.threads_init()
        gtk.main()

    #quitt Application
    def QuittApp(self,widget, data=None):

        Global.DatabaseConnection.close()

        if(self.__CurProjectObj <> None):
            if(self.__CurProjectObj.isChanged == True):
                msgbox = gtk.MessageDialog(parent = self.window, buttons = gtk.BUTTONS_YES_NO,
                                           flags = gtk.DIALOG_MODAL, type = gtk.MESSAGE_QUESTION,
                                           message_format = Global.QUITTWITHOUTSAVE )

                msgbox.set_title(Global.QUITMSGTITLE)
                result = msgbox.run()
                msgbox.destroy()
                if result == gtk.RESPONSE_YES:
                    pass
                else:
                    #Quitt without saving
                    gtk.main_quit()
            else:
                gtk.main_quit()
        else:
            gtk.main_quit()
