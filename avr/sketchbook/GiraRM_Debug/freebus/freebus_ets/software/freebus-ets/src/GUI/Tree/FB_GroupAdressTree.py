#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_GroupAdressTree.py
#Version: V0.1 , 01.06.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
import pygtk
from Global import Global
pygtk.require("2.0")
import gtk
import gtk.glade

##general class for creating/intializing our group-adress-tree
class FB_GroupAdressTree:

    __LogObj = None
    __treestore = None
    __TreeObj = None
    __CurProjectObj = None
    text_cell = None            #public object for treeview -> textcellrenderer
    img_cell = None             #public object for treeview -> cellrenderer
    column = None               #public object for treeview -> treeview column

    ArchModel = None

       #tree-structure:



    ##Constructor
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param PrjObj: Project Object
    def __init__(self,LogObj, TreeObj):
        self.__LogObj = LogObj
        self.__TreeObj = TreeObj

        #create data structure/model

        self.__treestore = gtk.TreeStore(gtk.gdk.Pixbuf,str,str)
        self.__TreeObj.set_model(self.__treestore)
        image=gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "maingroup.png")
        self.__TreeIterator = self.__treestore.append(None, [image, "  Hauptgruppen", "OK"])

        #get TreePath for TreeRowReference
        #TreePath = self.__treestore.get_path(self.__TreeIterator)
        #for later use with gtk 2.4 or higher
        #treerowref = TreeRowReference(TreeObj.get_model(), TreePath)

        self.text_cell = gtk.CellRendererText()            #Text Object
        self.img_cell = gtk.CellRendererPixbuf()           #Image Object
        self.column = gtk.TreeViewColumn()
        self.column.pack_start(self.img_cell, False)
        self.column.pack_start(self.text_cell,True)
        self.column.add_attribute(self.img_cell, "pixbuf",0)
        self.column.add_attribute(self.text_cell, "text", 1)
        self.column.set_attributes(self.text_cell, markup=1)
        self.__TreeObj.append_column(self.column)                #add objects t ofirst line of tree
        self.__TreeObj.expand_all()
        #text_cell.set_property("editable",True)
        # Allow drag and drop reordering of rows
        #TreeObj.set_reorderable(True)
        #self.__TreeObj.enable_model_drag_dest([('text/plain', 0,Global.DND_BUILDING)],gtk.gdk.ACTION_COPY)

    def ClearTree(self):
        self.__treestore.clear()
        self.__TreeObj.set_model(self.__treestore)
        image = gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "maingroup.png")
        self.__TreeIterator = self.__treestore.append(None, [image, "  Hauptgruppen", "OK"])

    #build the project tree, every time you create a new project or open an existing project
    #should be developed a better solution.... ;-)
    def CreateNewTree(self, ProjectObj):

        #set current project object
        self.__CurProjectObj = ProjectObj
        #object of our achrchitect
        self.ArchModel = self.__CurProjectObj.getArchModel()

        #set iterator to first position
        GroupAdressIter = self.__treestore.get_iter_first()

        #get root node
        Node = self.ArchModel.getDataRootNode(self.ArchModel.getRootID())

        #Column 1
        #self.__treestore.set_value(TopologyIter,1,self.__CurProjectObj.getProjectName())
        #Column 2
        self.__treestore.set_value(GroupAdressIter,2,self.ArchModel.getRootID())

        Obj = self.ArchModel.getDOMObj()

        #get List of ID of given Prefix (building-1,building-2,...)
        IDList = self.ArchModel.getIDList(Node,self.ArchModel.GROUPADRESS_MAIN)

        #for all groupadress maingroups....
        for mains in range(len(IDList)):
            LastMainGroupIter = self.CreateTreeNode(IDList[mains],GroupAdressIter,self.ArchModel.GROUPADRESS_MAIN)

            #get all middlegroups in given maingroup
            MiddleGroupNode = self.ArchModel.getDataRootNode(IDList[mains])
            MiddleGroupList = self.ArchModel.getIDList(MiddleGroupNode,self.ArchModel.GROUPADRESS_MIDDLE)
            #save last Iter
            MiddleGroupIter = LastMainGroupIter

            #for all groupadress middlegroups
            for middle in range(len(MiddleGroupList)):
                LastMiddleGroupIter = self.CreateTreeNode(MiddleGroupList[middle],MiddleGroupIter,self.ArchModel.GROUPADRESS_MIDDLE)
                #get all middlegroups in given maingroup
                SubGroupNode = self.ArchModel.getDataRootNode(MiddleGroupList[middle])
                SubGroupList = self.ArchModel.getIDList(SubGroupNode,self.ArchModel.GROUPADRESS)
                #save last Iter
                SubGroupIter = LastMiddleGroupIter

                #for all groupadress subgroups
                for sub in range(len(SubGroupList)):
                    LastSubGroupIter = self.CreateTreeNode(SubGroupList[sub],SubGroupIter,self.ArchModel.GROUPADRESS)


        self.__TreeObj.expand_all()


    def CreateTreeNode(self,ID,Iterator,Prefix):
        ParentNode = self.ArchModel.getDataRootNode(ID)

        #attr. will be saved in tree without visibilty
        Attr = self.ArchModel.getChildID(ParentNode)
        Value = self.ArchModel.readDOMNodeValue(ParentNode,"name")

        image = self.getImage(Prefix)

        iter =  self.__treestore.append(Iterator, [image, unicode(Value,"ISO-8859-1"), Attr])

        #Iterator should be None if Root-Node was selected
        if(Iterator <> None):
            path = self.__treestore.get_path(Iterator)
            self.__TreeObj.expand_row(path, True)

        self.__CurProjectObj.isChanged = True
        return iter


    def getImage(self,Prefix):

        if(Prefix == self.ArchModel.GROUPADRESS_MAIN):
            return gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "MainGroup.png")
        elif(Prefix == self.ArchModel.GROUPADRESS_MIDDLE):
            return gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "MiddleGroup.png")
        elif(Prefix == self.ArchModel.GROUPADRESS):
            return gtk.gdk.pixbuf_new_from_file(Global.ImagePath + "SubGroup.png")

    #gets the iterator of a given path ( comes from a drop action)
    def getIterator(self,path):
        return self.__treestore.get_iter(path)

    def getTreeValue(self,Iterator, position):
        return self.__treestore.get_value(Iterator, position)