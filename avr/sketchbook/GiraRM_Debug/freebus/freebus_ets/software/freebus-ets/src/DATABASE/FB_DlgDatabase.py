#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_DlgDatabase.py
#Version: V0.1 , 22.03.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

from Global import Global
import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade
#from configobj import ConfigObj            #for application settings
import os
from FB_PROJECT import configobj

import sqlite3

class FB_DlgDatabase:

    __LogObj = None
    __GladeObj = None                #general object as glade interface
    __window = None
    __txtDatabase = None            #widget of the textfield: Database


    def __init__(self, LogObj):
    # create a new window
        self.__LogObj = LogObj

        config = configobj.ConfigObj(Global.settingFile)

        self.__GladeObj = gtk.glade.XML(Global.GUIPath + "freebus.glade","DlgDatabse")
        self.__window = self.__GladeObj.get_widget("DlgDatabse")

        dic = { "on_bCancel_clicked" : self.Cancel,
                "on_bChooseDatabase_clicked" : self.FileChooser,
                "on_bApply_clicked" : self.Apply,
                "on_bNewDatabase_clicked" : self.NewDatabase,

                }
        self.__GladeObj.signal_autoconnect(dic)

        self.__txtDatabase = self.__GladeObj.get_widget("txtDatabase")
        self.__txtDatabase.set_text(config['Database']['Database'])
        #show window
        self.__window.show()

    #will be called after opening the filechooser dialog to choose the database
    def FileChooser(self,widget, data=None):

        DlgFileChooserGlade = gtk.glade.XML(Global.GUIPath  + "freebus.glade","DlgFileChooser")
        DlgFileChoose = DlgFileChooserGlade.get_widget("DlgFileChooser")

        #create file-filter
        filter = gtk.FileFilter()
        filter.add_pattern("*.db")
        filter.set_name("Freebus Datenbanken")
        DlgFileChoose.add_filter(filter)

        response = DlgFileChoose.run()

        if(response == gtk.RESPONSE_OK):
            self.__SelFolder = DlgFileChoose.get_current_folder()
            FileFolder = DlgFileChoose.get_filename()
            self.__txtDatabase.set_text(unicode(FileFolder,"ISO-8859-1"))

            DlgFileChoose.destroy()
        else:
            DlgFileChoose.destroy()

    #opens the filenew dialog to set the name for a new database
    def NewDatabase(self,widget, data=None):
        DlgNewDatabaseGlade = gtk.glade.XML(Global.GUIPath  + "freebus.glade","New_Database")
        wDlgNewDatabse = DlgNewDatabaseGlade.get_widget("New_Database")

        #get the textentry widget to get the name of the database
        txtDBName = DlgNewDatabaseGlade.get_widget("txtDatabaseName")
        #get the filechooser widget to get the choosen folder of the database
        wChooseFolder = DlgNewDatabaseGlade.get_widget("FileChooser")

        response = wDlgNewDatabse.run()
        if(response == gtk.RESPONSE_OK):

            DBFolder = wChooseFolder.get_current_folder()
            #try to find a "." inside the txtDBName , if so, then cut the name and add the ".db" extension
            newDBName = txtDBName.get_text()
            newDBName = newDBName.split(".")

            newDBName = DBFolder + "\\" + newDBName[0] + ".db"
            #check if database already exist
            if os.path.exists(newDBName):
                #
                msgbox = gtk.MessageDialog(parent = self.__window, buttons = gtk.BUTTONS_YES_NO,
                                           flags = gtk.DIALOG_MODAL, type = gtk.MESSAGE_WARNING,
                                           message_format = Global.DATATBASEEXIST )

                msgbox.set_title(Global.DATATBASEEXISTTITLE)
                result = msgbox.run()
                msgbox.destroy()
                if result == gtk.RESPONSE_YES:
                    #overwrite an existing database
                    self.CreateNewDatabase(newDBName)
                else:
                    #do nothing else at answer "NO"
                    pass

            else:
                self.CreateNewDatabase(newDBName)

            wDlgNewDatabse.destroy()
        else:
            wDlgNewDatabse.destroy()

    #applies the new settings (choose a new database)
    def Apply(self,widget, data=None):
        if(self.__txtDatabase != None):
            config = configobj.ConfigObj()
            config.filename = Global.settingFile
            config['Database'] = {}
            config['Database']['Database'] = self.__txtDatabase.get_text()
            config.write()

            #create a new connection object
            #test if Database is already open...
            try:
                Global.DatabaseConnection.close()
            except:
                pass
            Global.DatabaseConnection = sqlite3.connect(self.__txtDatabase.get_text())
            Global.DatabaseConnection.text_factory = str

        self.__window.destroy()

    def Cancel(self,widget, data=None):
        gtk.Widget.destroy(self.__window)

    #-------------------------------------------------------------------------------------------------------

    #creates an empty new Database of the latest version
    def CreateNewDatabase(self, DB_FileFolder):
        #create a new Database
        Global.DatabaseConnection = sqlite3.connect(DB_FileFolder)
        #and set the new name and folder
        self.__txtDatabase.set_text(DB_FileFolder)

        try:
            #create now the new database
            #to do this we will read out an new vd3 file to get the structure
            #open the external file to read out the sql create commands
            DBCreate_datei = Global.dataPath + "DBCreate.txt"
            InFileObj = open(DBCreate_datei,"r")

            CMDs = InFileObj.readlines()

            for i in range(len(CMDs)):
                cur = Global.DatabaseConnection.cursor()
                cur.execute(CMDs[i])

            InFileObj.close()

        except:
            #LOG File
            self.__LogObj.NewLog("Error at creating New Database...",1)

    def VDUnZip(self, VD_FileFolder):
        pass
