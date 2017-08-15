#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_NewProjectWindow.py
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


class FB_NEWPROJECTWINDOW:

    __WindowWidth = 0
    __WindowHeigth = 0
    __window = None
    __TreeView = None
    __SelFolder = ""    #selected Folder for Project
    __SelFile = ""      #selected Filename for Project
    __LogObj = None
    __Project = None    #general Project Object
    __MainFrame = None  #MainFrame Object
    __GladeObj = None   #general object to glade interface

    def __init__(self, LogObj,MainFrame):
        self.__LogObj = LogObj
        self.__MainFrame = MainFrame

        self.__GladeObj = gtk.glade.XML(Global.GUIPath + "freebus.glade","New_Project")

        self.__window = self.__GladeObj.get_widget("New_Project")

        dic = { "on_New_Project_destroy" : self.CloseWindow ,
                "on_bCancel_clicked" : self.bCancel,
                "on_bOK_clicked" :self.bOK}

        self.__GladeObj.signal_autoconnect(dic)
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
        w1 = self.__GladeObj.get_widget("FileChooser")
        w2 = self.__GladeObj.get_widget("editProjectName")

        checkProjDir = self.__GladeObj.get_widget("checkCreateProjDir")

        self.__SelFile = w2.get_text()
        self.__SelFolder = w1.get_current_folder()

        #check if user wants to create a projject directory
        checked = True #checkProjDir.get_active()
        NewPath = self.__SelFolder + "\\" + self.__SelFile

        os.chdir(self.__SelFolder)

        if(checked == True and self.__SelFile <> ""):
           #check if path already exist
           if(os.path.exists(NewPath) == True):
               self.__LogObj.NewLog("Try to create an existing directory for a project: Directory already exist! ",1)
           #create new directory
           else:
               os.mkdir(NewPath)
               os.chdir(NewPath)

        #create Projectstructure to current working direytroy
        self.__Project = FB_Project.FB_Project(self.__LogObj,self.__SelFile ,None)
        #set path of project
        self.__Project.setProjectPath(os.getcwd())

        #give back current project object
        self.__MainFrame.SetCurrProject(self.__Project)
        #close Dialog
        gtk.Widget.destroy(self.__window)


    def getProject(self):
        return self.__Project
