#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_XML_PRODUCT.py
#Version: V0.1 , 10.11.2007
#Version: V0.2 , 23.05.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import codecs
import xml.sax.handler
from Global import Global

import zipfile
import sqlite3

import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade
import threading
import thread
import os

from xml.dom import minidom

from xml.dom.minidom import *
from FB_DATA import FB_Constants



##This class contains all Methods to work with the FB-XML Files
#For each xml-product file you have to create an instance of "FB_XML"
#in this version the sax-parser will be used
class FB_XML_PRODUCT:

    __LogObj = None
    __DOMObj = None
    __xml_file = ""
    __handler = -1     #instance of XML-handler

    Finisch = False

    lblManufacturer = None
    lblProducts = None
    lblApps = None
    lblComObj = None
    lblParam = None

    __choosenImportType = 0    #choosen type to import (0=vd*; 1=vd_; 2=xml)
    __SelectedFile = ""        #selected folder+file from filedialog

    ##Constructor
    #for each product file you have to create an instance of FB_XML
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param file: Path and Filename of the xml-data-file
    def __init__(self,LogObj):
        self.__LogObj = LogObj

        self.DlgImportGlade = gtk.glade.XML(Global.GUIPath  + "freebus.glade","DlgProductImport")
        self.ImportDlg = self.DlgImportGlade.get_widget("DlgProductImport")

        #get widgets of textfields
        self.__txtSourceFile = self.DlgImportGlade.get_widget("txtSourceFile")
        self.__progress = self.DlgImportGlade.get_widget("ImportProgress")

        self.lblManufacturer = self.DlgImportGlade.get_widget("ManufacturerCount")
        self.lblProducts = self.DlgImportGlade.get_widget("Product_Count")
        self.lblApps = self.DlgImportGlade.get_widget("App_Count")
        self.lblComObj = self.DlgImportGlade.get_widget("Com_Count")
        self.lblParam = self.DlgImportGlade.get_widget("Param_Count")

        Value = "0 Stück"

        self.lblManufacturer.set_text(unicode(Value,"ISO-8859-1"))
        self.lblProducts.set_text(unicode(Value,"ISO-8859-1"))
        self.lblApps.set_text(unicode(Value,"ISO-8859-1"))
        self.lblComObj.set_text(unicode(Value,"ISO-8859-1"))
        self.lblParam.set_text(unicode(Value,"ISO-8859-1"))

        #fill combobox for change importtype
        self.__cbImportType = self.DlgImportGlade.get_widget("cbImportType")
        #set List-Items
        data=["*.VD*","*.VD_","*.XML"]
        store=gtk.ListStore(str)
        self.__cbImportType.set_model(store)

        cellrenderer = gtk.CellRendererText()
        self.__cbImportType.pack_start(cellrenderer)
        self.__cbImportType.add_attribute(cellrenderer, 'text', 0)

        for d in data:
            store.append([d])

        self.__cbImportType.set_active(0)


        self.__SelFolder = ""
        self.__SelFile = ""
        self.Finisch = False

        dic = {"on_bSelect_clicked": self.FileChooser,
               "on_bCancel_clicked": self.bCancel,
               "on_bImport_clicked": self.bImport }

        self.DlgImportGlade.signal_autoconnect(dic)


        self.ImportDlg.show()

    #open FileChooser
    def FileChooser(self, widget, data=None):

        DlgFileChooserGlade = gtk.glade.XML(Global.GUIPath  + "freebus.glade","DlgFileChooser")
        DlgFileChooser = DlgFileChooserGlade.get_widget("DlgFileChooser")

        #create file-filter depending on the choosen import type
        filter = gtk.FileFilter()
        #get the current choosen item
        curItem = self.__cbImportType.get_active() #_text()
        self.__choosenImportType = curItem

        #VD* (Original)
        if(curItem == 0):
            filter.add_pattern("*.vd[1-4]")
            filter.set_name("VD* Produktdaten")

        #VD_ (pre-unzipped via ETS)
        elif(curItem == 1):
            filter.add_pattern("*.vd_")
            filter.set_name("VD_ Produktdaten")
        #converted XML
        elif(curItem == 2):
            filter.add_pattern("*.xml")
            filter.set_name("XML Produktdaten")
        else:
            filter.add_pattern("*.vd[1-4]\*.vd_\*.xml")
            filter.set_name("Produktdaten")


        DlgFileChooser.add_filter(filter)

        response = DlgFileChooser.run()

        if(response == gtk.RESPONSE_OK):
            self.__SelFolder = DlgFileChooser.get_current_folder()
            self.__SelectedFile = DlgFileChooser.get_filename()

            #split SelFile (get_filename return complete path + filename)
            List = self.__SelectedFile.split("\\")
            tFileName = List[len(List)-1]
            tFileName = tFileName.split(".")

            #Folder and Filename complete

            self.__txtSourceFile.set_text(self.__SelectedFile)

            DlgFileChooser.destroy()
        else:
            DlgFileChooser.destroy()

    #close dialog
    def bCancel(self,widget, data=None):
        self.response = gtk.RESPONSE_CANCEL
        self.ImportDlg.destroy()


    #start convert
    def bImport(self,widget, data=None):

       # try:
               self.response = gtk.RESPONSE_OK

               productList = []
               appList = []
               ManufactList = []
               paramList = []

               Value = "0 Stück"

               self.lblManufacturer.set_text(unicode(Value,"ISO-8859-1"))
               self.lblProducts.set_text(unicode(Value,"ISO-8859-1"))
               self.lblApps.set_text(unicode(Value,"ISO-8859-1"))
               self.lblComObj.set_text(unicode(Value,"ISO-8859-1"))
               self.lblParam.set_text(unicode(Value,"ISO-8859-1"))

               #depending on the choosen import type do something...
               if(self.__choosenImportType == 0):
                   ErrorText = "Error at importing VD*-Type"
                   self.ExtractFromVDx()
               elif(self.__choosenImportType == 1):
                   ErrorText = "Error at importing VD_-Type"
                   self.ExtractFromVD_()
               elif(self.__choosenImportType == 2):
                   ErrorText = "Error at importing XML-Type"
                   self.ExtractFromXML()
               else:
                   return
        #except:
           # self.__LogObj.NewLog(ErrorText,0)

    def ExtractFromVDx(self):
        zip = zipfile.ZipFile(self.__SelectedFile,"r")
        for name in zip.namelist():
            if(name.find("vd_") > -1):
                outfile = open(Global.dataPath + "TempVD.vd_", 'wb')
                outfile.write(zip.read(name, 'Orleander'))
                outfile.close()
                #import it to the database
                self.__SelectedFile = Global.dataPath + "TempVD.vd_"
                self.ExtractFromVD_()
                os.remove(self.__SelectedFile)

        zip.close()


    def ExtractFromVD_(self):
        #source converted from C# ---> thanks to kubi
        InFileObj = open(self.__SelectedFile,"r")

        vdline = ""
        lineno = 0
        delimit = [ ' ' ]
        fieldcount = 0
        command = ""
        fieldType = ["" for i in range(50)]

        #create cursor
        cur = Global.DatabaseConnection.cursor()

        while(True):
            vdline = InFileObj.readline()

            if(vdline.find("XXX") > -1):
                Global.DatabaseConnection.commit()
                break

            else:
                #lineno = lineno + 1
                vdField = vdline.split(' ')

                if(len(vdField) > 2):
                    #table found
                    command = "CREATE TABLE IF NOT EXISTS " + vdField[2] + " ( id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "

                    if (vdField[0] == "T"):

                        fieldcount=0
                        vdline = InFileObj.readline()

                        while(vdline[0:1] == "C"):
                            vdField = vdline.split(' ')

                            if (vdField[0][1] !="1"):
                                command = command +", "
                            command = command + vdField[5]

                            # type: integer
                            if (vdField[2]=="1" or vdField[2]=="2"):
                                command = command + " INTEGER"
                                fieldType[fieldcount] = "i"

                            # type: varchar
                            elif (vdField[2]=="3" or vdField[2]=="8"):
                                command = command + " VARCHAR(" + vdField[3] + ")"
                                fieldType[fieldcount]="s"

                            # type: Real
                            elif (vdField[2]=="5"):
                                command = command + " REAL"
                                fieldType[fieldcount]="r"

                            fieldcount = fieldcount + 1

                            #store last-fileposition ...
                            LastPos = InFileObj.tell()
                            vdline = InFileObj.readline()

                        #restore last filepos if there was no more caption-section
                        InFileObj.seek(LastPos)
                        command = command +  ")"

                        #take command as sql statement
                        command = ""


                    if (vdField[0] == "R"):
                        #record found
                        command = "INSERT INTO " + vdField[4] + " VALUES(null, "
                        SelCommand = "SELECT * FROM " + vdField[4]
                        ValueList = ["null"]

                        n = 0
                        while(n < fieldcount):
                            vdline = InFileObj.readline()

                            if (len(vdline) > 1 and vdline[0:2] == "\\\\"):
                                n = n - 1
                                command = command[0:len(command)-1]
                                ValueList.pop()
                            else:
                                if (n > 0):
                                    command = command + ", "
                                if (fieldType[n]=="s"):
                                    command = command + "'"

                            if (vdline != "\n"):

                                tmp = vdline.replace("\\\'","")
                                tmp = tmp.split("\n")
                                ValueList.append(unicode(tmp[0],"iso-8859-1"))

                                command = command + unicode(tmp[0],"iso-8859-1")
                                if (fieldType[n] == "s"):
                                    command = command + "'"
                            else:
                                #in case of empty line
                                if(fieldType[n] == "i"):
                                    command = command + "0"
                                    ValueList.append("0")

                                elif (fieldType[n] == "s"):
                                    command = command + "'"
                                    ValueList.append(" ")

                                elif (fieldType[n] == "r"):
                                    command = command + "0.0"
                                    ValueList.append("0.0")

                            n = n + 1


                        command = command + ")"

                        #take command as sql statement
                        #try:
                        #check if record is already in database
                        if (self.CheckSQLRecord(SelCommand,ValueList,fieldType) == False):
                            cur.execute(command)

                       # except:
                          #  print command
                         #   break
                        #command = ""



    def ExtractFromXML(self):
        pass

    ##check the database if the record which should be inserted is already not in database
    #@param SelCommand: SQL Select-Command of current table
    #@param ValueList: current List of Values which should be inserted
    #@param TypeList: current TypeList of inserted values (integr, strings ...)
    def CheckSQLRecord(self,SelCommand,ValueList,TypeList):
        RValue = False

        #create new cursor
        Global.DatabaseConnection.row_factory = sqlite3.Row
        c = Global.DatabaseConnection.cursor()
        c.execute(SelCommand)
        r = c.fetchone()

        if r <> None:
            #get columnlist
            columnList = r.keys()

            #print columnList
            #print ValueList
            #print TypeList
            SQLStr = SelCommand + " WHERE "
            #create new select-string starting at index 1 at ValueList (Index 0 = ID which is not to compare)
            for n in range(1,len(columnList)):
                if TypeList[n-1] == 'i':
                    if(n == 1):
                        SQLStr = SQLStr + columnList[n] + " = " + ValueList[n]
                    else:
                        SQLStr = SQLStr + " AND " +  columnList[n] + " = " + ValueList[n]

            c = Global.DatabaseConnection.cursor()
           # print SQLStr
            c.execute(SQLStr)


            if(len(c.fetchall()) > 0):
                RValue = True
            else:
                pass

        return RValue


    def worker_thread(self, process_import):

        # hier wird gearbeitet
        if(process_import <> None):
            XMLHandler = process_import()
            if(XMLHandler != -1):
                print "OK"


    ##return the current DOM-Object
    def getDOMObj(self):
        self.__DOMObj = parse(self.__xml_file)
        return self.__DOMObj



class ProgressBar(threading.Thread):

    def __init__(self,BarObj,Converter,Dlg):
        threading.Thread.__init__(self)
        self.BarObj = BarObj
        self.Converter = Converter
        self.Dlg = Dlg


    def run(self):
        gtk.gdk.threads_enter()
        while self.Converter.Finisch == False:

            #self.Dlg.set_cursor(gtk.gdk.Cursor(gtk.gdk.HAND1))
            if(self.Converter.LineCount > 0):
                self.visu = "%3.2f" % (float(str(self.Converter.CurLine)) / float(str(self.Converter.LineCount)))
                #print self.visu
                if(self.Converter.CurLine > self.Converter.LineCount):
                    self.visu = 1.0
                self.BarObj.set_fraction(float(self.visu))
                while gtk.events_pending():
                    gtk.main_iteration(False)
                sleep(0.02)
        gtk.gdk.threads_leave()

