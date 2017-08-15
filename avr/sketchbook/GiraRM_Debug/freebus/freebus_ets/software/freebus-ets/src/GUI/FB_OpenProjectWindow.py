#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_OpenProjectWindow.py
#Version: V0.1 , 17.02.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
import sys
from Global import Global
import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade
from GUI import *
from FB_PROJECT import FB_ArchitecturalDataModel
from FB_PROJECT import FB_Project


class FB_OPENPROJECTWINDOW:

    __WindowWidth = 0
    __WindowHeigth = 0
    __wTree = None
    __window = None
    __TreeView = None
    __SelFolder = ""    #selected Folder for Project
    __SelFile = ""      #selected Filename for Project
    __LogObj = None
    __Project = None    #general Project Object
    __MainFrame = None  #MainFrame Object

    def __init__(self, LogObj, MainFrame):
        self.__LogObj = LogObj
        self.__MainFrame = MainFrame

        GladeObj = gtk.glade.XML(Global.GUIPath + "freebus.glade","Open_Project")

        self.__window = GladeObj.get_widget("Open_Project")

        self.__window.show_all()

        dic = { "on_Open_Project_destroy" : self.CloseWindow ,
                "on_bCancel_clicked" : self.bCancel,
                "on_bOK_clicked" :self.bOK}

        GladeObj.signal_autoconnect(dic)

        #create file-filter
        filter = gtk.FileFilter()
        filter.add_pattern("structure.xml")
        filter.set_name("Freebus XML")
        self.__window.add_filter(filter)

        self.__SelFolder = ""
        self.__SelFile = ""

    def CloseWindow(self,widget):
        gtk.Widget.destroy(self.__window)

    #Cancel Button has been pressed
    def bCancel(self,widget):
        gtk.Widget.destroy(self.__window)

    #OK Button has been pressed
    def bOK(self,widget):
        #get data from FileChooser

        #check if choosen file is a correct freebus structure file
        self.__SelFolder = self.__window.get_current_folder()
        FileFolder = self.__window.get_filename()

        #split SelFile (get_filename return complete path + filename)
        List = FileFolder.split("\\")
        tFileName = List[len(List)-1]

        #check for xml file
        if(tFileName.find(".xml") <> -1):
            self.__SelFile = tFileName
            #create Projectstructure to current working direytroy
            self.__Project = FB_Project.FB_Project(self.__LogObj,self.__SelFile ,self.__SelFolder)
            self.__Project.setProjectPath(self.__SelFolder)
            self.__MainFrame.SetCurrProject(self.__Project)
        else:
            self.__Project = None
            self.__LogObj.NewLog("There was no project file choosen; Please choose a valid freebus xml structure file. ",1)

        #close Dialog
        #give back current project object

        gtk.Widget.destroy(self.__window)

    def getProject(self):
        return self.__Project
