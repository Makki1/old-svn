#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_DlgDeviceData.py
#Version: V0.2 , 11.06.2009
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


from Global import Global
import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade
from FB_DATA import FB_ParameterHandling
from FB_DATA import FB_Device

class FB_DlgDeviceData:

    __LogObj = None
    __CurProject = None
    __ArchProj = None
    __InstProj = None
    __GladeObj = None                #general object as glade interface
    __window = None

    #objects for manufacturer handling
    __ManufacturerList = None             #widget of Manufacturer List
    __ManufacturerListStore = None        #ListModel interface
    __ManufacturerListSelection = None    #Selection of List
    __SelectedManufacturer = 0            #current selected Manufacturer

    #object for product handling
    __productList = None             #widget of product List
    __productFrameLabel = None       #widget of FrameLabel of product List
    __productListStore = None        #ListModel interface
    __productListSelection = None    #Selection of List
    __SelectedProduct = 0            #item number current selected Product
    __ProductNameList = None         #Product of Database Content

    #object for Application handling
    __appList = None             #widget of Application List
    __lblApps = None             #widget of FrameLabel ao App-box
    __lblApplicationName = None  #widget of Info Label Application-Name
    __lblProgramVersion = None   #widget of Info Label Program Version

    __appsListStore = None       #ListModel interface
    __appListSelection = None    #Selection of List
    __SelectedApplication = 0    #current selected Application (ListRow)
    __ApplicationNameList = []   #Application of Database Content

    #object for Communications objects handling
    __commObjList = None         #widget of Communication Object List
    __commObjListStore = None    #ListModel interface


    __lblDeviceAdress = None    #widget of the device-Adress label
    __spinDeviceAdress = None   #widget ofthe spinbutton for device Adress selection
    __lastValidAdress = 0

    __OnlyShow = None
    __ParentID = None           #ID of Device-Parent (to which Parent should belong this Device )
    __EventHandler = None
    __AdressLogicObj = None     #instance to the adress-logic class
    __CurProductName = ""
    __TopologyTree = None        #object of the topology tree
    __Iterator = None            #iterator of the topology tree

    ##Construcotr to create a Dialog to show the deivce data of database
    #@param LogObj: Loggig Object to log any exceptions
    #@param OnlyShow: there are two modes of this dialog: show -> just show the data without any adress fields;
    #show = false -> show the adress-spin field as well
    def __init__(self, LogObj,AdressLogic, OnlyShow):
    # create a new window
        self.__LogObj = LogObj
        self.__OnlyShow = OnlyShow
        self.__AdressLogicObj = AdressLogic

        self.__GladeObj = gtk.glade.XML(Global.GUIPath + "freebus.glade","DeviceDataWindow")
        self.__window = self.__GladeObj.get_widget("DeviceDataWindow")

        #Init ManufacturerList
        self.InitManufacturerList()
        #Init ProductList
        self.InitProductList()
        #Init Application List
        self.InitAppList(OnlyShow)
        #Init Communication Object List
        self.InitCommObjList()

        dic = { "on_bApply_clicked" : self.Apply,
                "on_bClose_clicked" : self.Close,
                "on_ListManufacturer_cursor_changed" : self.ListManufacturerChange,
                "on_ListProducts_cursor_changed" : self.ListProductsChange,
                "on_ListApps_cursor_changed" : self.ListAppsChange,
                }
        self.__GladeObj.signal_autoconnect(dic)

        self.__CurProductName = ""

        #fill manufacturerlist
        self.UpdateManufacturerList()



        #show window
        self.__window.show()



       #-------------------------------------------------------------------------------------
    def InitManufacturerList(self):
        #init product_list
        self.__ManufacturerList = self.__GladeObj.get_widget("ListManufacturer")
        #create ListStore object with 1xpicture and 1xstring entry
        self.__ManufacturerListStore = gtk.ListStore(gtk.gdk.Pixbuf, str,str)
        self.__ManufacturerList.set_model(self.__ManufacturerListStore)
        self.text_cell = gtk.CellRendererText()            #Text Object
        self.img_cell = gtk.CellRendererPixbuf()           #Image Object
        self.column = gtk.TreeViewColumn()
        self.column.pack_start(self.img_cell, False)
        self.column.pack_start(self.text_cell,True)
        self.column.add_attribute(self.img_cell, "pixbuf",0)
        self.column.add_attribute(self.text_cell, "text", 1)
        self.column.set_attributes(self.text_cell, markup=1)
        self.__ManufacturerList.append_column(self.column)                #add objects t ofirst line of tree


    def InitProductList(self):
        #init product_list
        self.__productList = self.__GladeObj.get_widget("ListProducts")
        #create ListStore object with 1xpicture and 1xstring entry and 1xstring ProductID
        self.__productListStore = gtk.ListStore(gtk.gdk.Pixbuf, str,str)
        self.__productList.set_model(self.__productListStore)
        self.text_cell = gtk.CellRendererText()            #Text Object
        self.img_cell = gtk.CellRendererPixbuf()           #Image Object
        self.column = gtk.TreeViewColumn()
        self.column.pack_start(self.img_cell, False)
        self.column.pack_start(self.text_cell,True)
        self.column.add_attribute(self.img_cell, "pixbuf",0)
        self.column.add_attribute(self.text_cell, "text", 1)
        self.column.set_attributes(self.text_cell, markup=1)
        self.__productList.append_column(self.column)                #add objects t ofirst line of tree

        lblProducts = self.__GladeObj.get_widget("lblProducts")
        lblProducts.set_markup(unicode("<b>Produkte</b>" ,"ISO-8859-1"))

    def InitAppList(self,OnlyShow):
        #init app-List
        self.__appList = self.__GladeObj.get_widget("ListApps")
        #create ListStore object with 1xpicture and 1xstring AppName and 1xstring ProgramID
        self.__appsListStore = gtk.ListStore(gtk.gdk.Pixbuf, str,str)
        self.__appList.set_model(self.__appsListStore)
        self.text_cell = gtk.CellRendererText()            #Text Object
        self.img_cell = gtk.CellRendererPixbuf()           #Image Object
        self.column = gtk.TreeViewColumn()
        self.column.pack_start(self.img_cell, False)
        self.column.pack_start(self.text_cell,True)
        self.column.add_attribute(self.img_cell, "pixbuf",0)
        self.column.add_attribute(self.text_cell, "text", 1)
        self.column.set_attributes(self.text_cell, markup=1)
        self.__appList.append_column(self.column)                #add objects t ofirst line of tree

        #Frame Label
        self.__lblApps = self.__GladeObj.get_widget("lblApps")
        self.__lblApps.set_markup(unicode("<b>Applikationen</b>" ,"ISO-8859-1"))
        #Info Label...
        self.__lblApplicationName = self.__GladeObj.get_widget("lblApplicationName")
        self.__lblProgramVersion = self.__GladeObj.get_widget("lblProgramVersion")


        self.__lblApplicationName.set_text("")
        self.__lblProgramVersion.set_text("")

        #adress spin fields
        #get the widgets...
        self.__lblDeviceAdress = self.__GladeObj.get_widget("lblDeviceAdress")
        self.__spinDeviceAdress = self.__GladeObj.get_widget("spinDeviceAdress")
        ApplyButton = self.__GladeObj.get_widget("bApply")

        if(OnlyShow == True):
            self.__lblDeviceAdress.hide()
            self.__spinDeviceAdress.hide()
            ApplyButton.hide()

        else:
            self.__lblDeviceAdress.show()
            self.__spinDeviceAdress.show()
            ApplyButton.show()


    def InitCommObjList(self):
        #init Comm-obj List
        self.__commObjList = self.__GladeObj.get_widget("ListComObj")
        #create ListStore object with 1xpicture and 2xstring ObjName, ObjFunction and 1xstring ProgramID
        self.__commObjListStore = gtk.ListStore(gtk.gdk.Pixbuf, str,str,str)

        self.__commObjList.set_model(self.__commObjListStore)

        self.column1 = gtk.TreeViewColumn('Name')
        self.column2 = gtk.TreeViewColumn('Funktion')

        # create a CellRenderers to render the data
        self.cellpb = gtk.CellRendererPixbuf()
        self.text_cell1 = gtk.CellRendererText()
        self.text_cell2 = gtk.CellRendererText()

        self.column1.pack_start(self.cellpb, False)
        self.column1.pack_start(self.text_cell1,False)
        self.column2.pack_start(self.text_cell2,False)
        self.column1.add_attribute(self.cellpb, "pixbuf",0)
        self.column1.add_attribute(self.text_cell1, "text", 1)
        self.column2.add_attribute(self.text_cell2, "text", 2)

         #add to listview
        self.__commObjList.append_column(self.column1)                #add objects
        self.__commObjList.append_column(self.column2)                #add objects


    def UpdateManufacturerList(self):
        image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "ManufacturerChooser.png")

        #structure
        #id / Manufacturer_ID / Manufacturer_Name / Adress_ID

        #find out all of manufacturers
        sql = "SELECT * FROM manufacturer order by MANUFACTURER_ID"
        ListIterator = None

        Cursor = Global.DatabaseConnection.cursor()
        try:
            Cursor.execute(sql)
        except:
            pass

        ManuList = Cursor.fetchall()
        if(len(ManuList) > 0):
            for i in range(len(ManuList)):
                ListIterator = self.__ManufacturerListStore.append([image,ManuList[i][2],ManuList[i][1]])

        #no items found...
        else:
            pass


    def UpdateProductList(self,ManufacturerID):


        if(ManufacturerID <= 0):
            return

        #clear complete List
        self.__productListStore.clear()
        self.__appsListStore.clear()
        self.__commObjListStore.clear()
        #clear InformationLIst of Applications
        self.UpdateAppInformation(-1)

        Cursor = Global.DatabaseConnection.cursor()
        #structure
        #id / Product_ID / Manufacturer_ID / Symbol_ID / Product_Name ...

        #set frame label to current manufacturer
        sql = "SELECT * from manufacturer where MANUFACTURER_ID = " + ManufacturerID
        try:
            Cursor.execute(sql)
            ProductNameList = Cursor.fetchall()

            #first element of recordset (should only be one!)
            ProductName = "<b>Produkte von: " + ProductNameList[0][2] + "</b>"

            lblProducts = self.__GladeObj.get_widget("lblProducts")
            lblProducts.set_markup(unicode(ProductName ,"ISO-8859-1"))

        except:
            pass


        image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "ProductChooser.png")

        #find out all of manufacturers
        sql = "SELECT * FROM catalog_entry where MANUFACTURER_ID = " + ManufacturerID
        ListIterator = None


        try:
            Cursor.execute(sql)

            if(self.__ProductNameList <> None):
                del self.__ProductNameList

            self.__ProductNameList = Cursor.fetchall()

               # print self.__ProductNameList
            if(len(self.__ProductNameList) > 0):
                for i in range(len(self.__ProductNameList)):
                    ListIterator = self.__productListStore.append([image,unicode(self.__ProductNameList[i][6],"iso-8859-1"),self.__ProductNameList[i][2]])

            #no items found...
            else:
                pass
        except:
            pass



    def UpdateAppList(self,ProductID):
        if(ProductID <= 0):
            return

        self.__ApplicationNameList = []

        #clear complete List
        self.__appsListStore.clear()
        self.__commObjListStore.clear()
        #clear InformationLIst of Applications
        self.UpdateAppInformation(-1)

        Cursor = Global.DatabaseConnection.cursor()

        #first: get program_id of given product_id:
        sql = "SELECT PROGRAM_ID from product_to_program where PRODUCT_ID = " + ProductID

        try:
            Cursor.execute(sql)
            ProdProgrList = Cursor.fetchall()

        except:
            pass

        #delete entire List
        del self.__ApplicationNameList[0:len(self.__ApplicationNameList)]

        #second: create Application List
        for i in range(len(ProdProgrList)):

            sql = "SELECT * from application_program where PROGRAM_ID = " + str(ProdProgrList[i][0])

            try:
                Cursor.execute(sql)
                TempList = Cursor.fetchall()

                #no zero-item ?
                if(len(TempList) > 0):
                    self.__ApplicationNameList.append(TempList)

            except:
                pass


        #new device requested
        if(self.__OnlyShow == False):
            #changed value at spinadress -> DlgStructureElement
            #connect event of spin-adress-button
            if(self.__EventHandler <> None):
                self.__spinDeviceAdress.disconnect(self.__EventHandler)
                #new connect

            self.__EventHandler = self.__spinDeviceAdress.connect("value_changed",self.spinAdressValueChanged, \
                                                                    (self.__ArchProj.TOPOLOGY_DEVICE,self.__ParentID,))
            #StartAdress to check = 1
            self.__lastValidAdress = self.__AdressLogicObj.GetNextPossibleAdress(self.__ArchProj.TOPOLOGY_DEVICE,1, True,self.__ParentID)
            self.__spinDeviceAdress.set_value(self.__lastValidAdress)


        image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "AppsChooser.png")

        #third: show Application List
        if(len(self.__ApplicationNameList) > 0):
            for i in range(len(self.__ApplicationNameList)):
                ListIterator = self.__appsListStore.append([image,unicode(self.__ApplicationNameList[i][0][4],"iso-8859-1"),self.__ApplicationNameList[i][0][1]])

        #no items found...
        else:
            pass

        #write frame Label to current Product
        sql = "SELECT * FROM catalog_entry where PRODUCT_ID = " + ProductID
        try:
            Cursor.execute(sql)
            AppList = Cursor.fetchall()

            AppName = "<b>Applikationen von: " + AppList[0][6] + "</b>"
            self.__lblApps.set_markup(unicode(AppName ,"ISO-8859-1"))
            #save current product name
            self.__CurProductName = AppList[0][6]

        except:
            pass

    #UpdateApp-Information Line
    #@param ListRow: List of application program to extract the version,...
    def UpdateAppInformation(self, ListRow):

        if(ListRow >= 0):
            #Info Label: App-Name
            self.__lblApplicationName.set_text(unicode("Applikation: " + self.__ApplicationNameList[ListRow][0][4]  ,"ISO-8859-1"))
            #Info Label: Program Version
            VersionHex = str(self.__ApplicationNameList[ListRow][0][5])
            Version = "%X" % int(VersionHex)
            Version = float(Version) / 10.0
            self.__lblProgramVersion.set_text("Program-Version: " + str(Version))

            #update the communication object list as well
            #clear list
            self.__commObjListStore.clear()

            #index 1 = program_id to identify the communication objects
            sql = "SELECT * FROM communication_object where PROGRAM_ID = " + str(self.__ApplicationNameList[ListRow][0][1]) \
                  + " AND PARENT_PARAMETER_VALUE = 0 ORDER BY OBJECT_DISPLAY_ORDER"


            try:
                Cursor = Global.DatabaseConnection.cursor()
                Cursor.execute(sql)
                CommObjList = Cursor.fetchall()

                image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "CommObj.png")

                if(len(CommObjList) > 0):
                    for i in range(len(CommObjList)):
                        #picture / Name [2] / Function [3]
                        ListIterator = self.__commObjListStore.append([image, CommObjList[i][2],CommObjList[i][3],CommObjList[i][1]])

            except:
                pass

        else:
            self.__lblApplicationName.set_text("")
            self.__lblProgramVersion.set_text("")




    #Selection of ManufacturerList has been changed...
    def ListManufacturerChange(self,widget,data=None):
        #get current selection
        self.__ManufacturerListSelection =  self.__ManufacturerList.get_selection()
        (model, iter) = self.__ManufacturerListSelection.get_selected()
        #get Manufacturer-ID (column = 2)
        self.__SelectedManufacturer = self.__ManufacturerListStore.get_value(iter, 2)
        #return Manufacturer ID
        self.UpdateProductList(self.__SelectedManufacturer)

    #Selection of Products has been changed
    def ListProductsChange(self, widget, data=None):
         #get current selection
        self.__productListSelection =  self.__productList.get_selection()
        (model, iter) = self.__productListSelection.get_selected()
        #get path to find out which item has been selected
        path = self.__productListStore.get_path(iter)
        #write item number into private variable
        self.__SelectedProduct = path[0]
        #return Product ID
        self.UpdateAppList(self.__productListStore.get_value(iter, 2))

    #selection of Application has been changed
    def ListAppsChange(self, widget, data=None):
        #get current selection
        self.__appListSelection =  self.__appList.get_selection()
        (model, iter) = self.__appListSelection.get_selected()
        path = self.__appsListStore.get_path(iter)
        #return item-number in listcontrol
        self.__SelectedApplication = path[0]
        #show communications-objects...
        self.UpdateAppInformation(self.__SelectedApplication)

    #change event of spin button (adress- changes)
    #@param data: PicturePrefix + ParentID  (PicturePrefix is just for combatibility reasons to the AdressLogic-class)
    def spinAdressValueChanged(self,widget, data):
        #the pictureprefix (data[0] should always be 'Device' here
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

    #----------------------------------------------------------------------------------------------------------------

    #property ParentID in case the device should add in the project
    def SetParentID(self,ID):
        self.__ParentID = ID

    def SetTopologyProperty(self,TopologyTree, Iterator):
        self.__TopologyTree = TopologyTree
        self.__Iterator = Iterator
    #----------------------------------------------------------------------------------------------------------------

    def SetProjectObj(self,ProjectObj):
        self.__CurProject = ProjectObj
        self.__ArchProj = ProjectObj.getArchModel()
        self.__InstProj = ProjectObj.getInstModel()

    #----------------------------------------------------------------------------------------------------------------

    def Close(self,widget, data=None):
        gtk.Widget.destroy(self.__window)

    #insert selected device....
    def Apply(self,widget, data=None):
        #apply is only visible at inserting mode of devices
        #add device entry in architecture tree
        #add complete device in installation file
        #add complete device in device class
        #--------------------------------------------------------------------------------------------------------------
        #1. get a new ID for the device
        #add object and gets the new ID (Device-1 ...)
        ID = self.__ArchProj.addChild(self.__ParentID)
        Name = self.__CurProductName

        if(ID <> None):
            self.__ArchProj.setName(ID,Name)
            self.__ArchProj.setAdress(ID, int(self.__spinDeviceAdress.get_value()))
            name = self.__ArchProj.getName(ID)
            self.__ArchProj.setName(ID,name + " (" + str(int(self.__spinDeviceAdress.get_value())) + ")")
        #add it to the topology tree-node
        self.__TopologyTree.CreateTreeNode(ID, self.__Iterator, self.__ArchProj.TOPOLOGY_DEVICE)

        #--------------------------------------------------------------------------------------------------------------
        #2. save the entire device information in installation file
        if(ID <> None):
            #create new device-entry in installation model mit the same device-id
            self.__InstProj.AddDevice(ID)

        #--------------------------------------------------------------------------------------------------------------
        #3. create a new instance of the device-class
        if(ID <> None):
            DeviceInstance = FB_Device.FB_Device(self.__LogObj,self.__CurProject)
            #write all urgent values
            DeviceInstance.WriteProductDetails()

            self.__InstProj.WriteDeviceInstance(ID,DeviceInstance)


        self.__CurProject.isChanged = True
        #close dialog
        gtk.Widget.destroy(self.__window)
