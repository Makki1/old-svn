#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_XMLConverter.py
#Version: V0.1 , 04.11.2007
#Version: V0.2 , 22.05.2008
#Version: V0.3 , 08.06.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
import time
import codecs
import locale
import gobject
from time import *
from re import *
import threading
import thread
import types

from Global import Global
import pygtk
pygtk.require("2.0")
import gtk
import gtk.glade
from GUI import *

##This class contains all Methods to convert a original *.vd_ File to its corresponding
##XML-File.


class FB_XMLConverter:


    data=""

    read_caption=False
    read_record=False
    read_table=False
    read_value=False
    atmp = []
    recordname=""
    captions = []
    val_count=0
    __LogObj = None
    lineCounter = 0
    ByteCount = 0
    Finisch = False
    DlgConvertGlade = None
    DlgConvert = None         #widget
    __txtSourceFile = None    #widget
    __txtDestFile = None      #widget
    __progress = None         #widget
    __SelFolder = ""
    __SelFile = ""
    __LineParsed = False

    response = 0
    LineCount = 0        #line count -> visu
    CurLine = 0
    visu = 0.0
    ProgressBar = None    #calssobj


    ## The constructor.
    #
    # @param self : The object pointer
    # @param InFile : Path and Filename of Input-File (*.vd_)
    # @param InFile : Path and Filename of Output-File (*.xml)
    # @param LogObj : a Object to Logging-Class
    def __init__(self,LogObj):#,jobqueue, ausschalter):

        self.DlgConvertGlade = gtk.glade.XML(Global.GUIPath  + "freebus.glade" ,"DlgConvertDeviceData")
        self.DlgConvert = self.DlgConvertGlade.get_widget("DlgConvertDeviceData")

        #get widgets of textfields
        self.__txtSourceFile = self.DlgConvertGlade.get_widget("txtSourceFile")
        self.__txtDestFile = self.DlgConvertGlade.get_widget("txtDestFile")
        self.__progress = self.DlgConvertGlade.get_widget("ConvertProgress")

        self.__SelFolder = ""
        self.__SelFile = ""
        self.Finisch = False

        dic = {"on_bSelect_clicked": self.FileChooser,
               "on_bCancel_clicked":self.bCancel,
               "on_bConvert_clicked":self.bConvert }

        self.DlgConvertGlade.signal_autoconnect(dic)

        self.DlgConvert.show()

        self.__LogObj = LogObj

    #open FileChooser
    def FileChooser(self, widget, data=None):

        DlgFileChooserGlade = gtk.glade.XML(Global.GUIPath  + "freebus.glade","DlgFileChooser")
        DlgFileChooser = DlgFileChooserGlade.get_widget("DlgFileChooser")

        #create file-filter
        filter = gtk.FileFilter()
        filter.add_pattern("*.vd_")
        filter.set_name("EIB Produktdaten")
        DlgFileChooser.add_filter(filter)

        response = DlgFileChooser.run()

        if(response == gtk.RESPONSE_OK):
            self.__SelFolder = DlgFileChooser.get_current_folder()
            FileFolder = DlgFileChooser.get_filename()

            #split SelFile (get_filename return complete path + filename)
            List = FileFolder.split("\\")
            tFileName = List[len(List)-1]
            tFileName = tFileName.split(".")

            #Folder and Filename complete
            self.vd_datei = FileFolder
            self.xml_datei = Global.dataPath + tFileName[0] + ".xml"

            self.__txtSourceFile.set_text(self.vd_datei)
            self.__txtDestFile.set_text(tFileName[0] + ".xml")

            DlgFileChooser.destroy()
        else:
            DlgFileChooser.destroy()

    #close dialog
    def bCancel(self,widget, data=None):
        self.response = gtk.RESPONSE_CANCEL
        self.DlgConvert.destroy()


    #start convert
    def bConvert(self,widget, data=None):

        self.response = gtk.RESPONSE_OK
        self.ProgressBar = ProgressBar(self.__progress,self,self.DlgConvert)
        self.ProgressBar.start()

        thread.start_new(self.worker_thread, (self.convertToXML(),))


    def worker_thread(self, process_convert):

        # hier wird gearbeitet
        if(process_convert <> None):
            process_convert()

    ## Base Method for *.vd_ File to FB-XML-File Conversion
    #
    #  @param self The object pointer.
    def convertToXML(self):

        parse=False
        line=""
        next=""
        OutFileObj = None
        InFileObj = None


        try:
            if(self.vd_datei != ""):
                OutFileObj = open(self.xml_datei,"w")
                InFileObj = open(self.vd_datei,"r")

                #LOG File
                self.__LogObj.NewLog(self.vd_datei+" geöffnet",0)
                self.__LogObj.NewLog(self.xml_datei+" geöffnet",0)

                #write first two lines
                OutFileObj.write("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n")

                OutFileObj.write("<eib-products>\n")

                #analyse file for visualisation
                try:
                    self.LineCount = len(InFileObj.readlines())

                except:
                    self.LineCount = 25000000

                InFileObj.seek(0)

                #print self.LineCount
                #read first line and compare of signum
                line = InFileObj.readline()
                line = line.split("\n")
                self.CurLine = 1

                if(line[0] != "EX-IM"):
                    #LOG File
                    self.__LogObj.NewLog("Falsches Format der *vd_ Datei gefunden",1)
                    return

                line = InFileObj.readline()
                line = line.split("\n")
                self.CurLine = self.CurLine + 1


                #normal parse-run
                while(True):

                   # print InFileObj.tell()
                    next = InFileObj.readline()
                    next = next.split("\n")
                    self.CurLine = self.CurLine + 1

                    if(line[0] == "XXX"):
                        self.parseLine(line[0],OutFileObj,1)

                    #cancel first While-Loop , if current line is empty
                        break

                    #parse Symbol-Information
                    re = compile('^\\\\')
                    while(re.match(next[0])):
                        #separate "\\"
                        line[0] = line[0] + next[0].strip("\\\\")

                        next = InFileObj.readline()
                        next = next.split("\n")

                        self.CurLine = self.CurLine + 1

                        if(next[0] == ""):
                            if(line[0] != ""):
                                self.parseLine(line[0],OutFileObj,2)
                                self.__LineParsed = True
                    #parse lines after symbolinformation

                    if(self.__LineParsed == False):
                        self.parseLine(line[0],OutFileObj,3)

                    self.__LineParsed = False
                    line=next


                OutFileObj.flush()
                OutFileObj.close()
                InFileObj.close()
                #LOG File
                self.__LogObj.NewLog(self.vd_datei+" geschlossen",0)
                self.__LogObj.NewLog(self.xml_datei+" geschlossen",0)
                self.Finisch = True
                self.__progress.set_fraction(0.0)
                self.DlgConvert.destroy()


        except IOError:

            #LOG File
            self.__LogObj.NewLog("Fehler beim Öffnen der vd_ Datei",2)

            if(OutFileObj != None):
                OutFileObj.close()

            if(InFileObj != None):
                InFileObj.close()

            #LOG File
            self.__LogObj.NewLog(self.vd_datei+" geschlossen",0)
            self.__LogObj.NewLog(self.xml_datei+" geschlossen",0)



        except:

            #LOG File

            if(self.CurLine > 0):
                self.__LogObj.NewLog("Allgemeiner Fehler beim Parsen der vd_ Datei" \
                                 + " in Zeile: " + str(self.lineCounter)  \
                                 +" " + line[0],2)
            else:
                self.__LogObj.NewLog("Allgemeiner Fehler beim Parsen der vd_ Datei" \
                                 + " in Zeile: " + str(self.lineCounter)  \
                                 +" " ,2)

            if(OutFileObj != None):
                OutFileObj.close()
                #LOG File
                self.__LogObj.NewLog(self.xml_datei+" geschlossen",0)

            elif(InFileObj != None):
                InFileObj.close()
                #LOG File
                self.__LogObj.NewLog(self.vd_datei+" geschlossen",0)


    def parseLine(self,_line,_OutFileObj,No):

        tmp=""

        #saftey: if line will be called twice to parse...

        try:
            #self.lineCounter = self.lineCounter + 1

            #line empty ?
            if(len(_line) > 0):
                #erstes Zeichen laden
                start=_line[0:1]


                #Start with "T " ? ->
                if(_line[0:2] == "T "):
                    start=' '

            else:
                _line=""

            if(_line[0:2] == "N " ):
                #Path of vd_ Datei

                leer = 0
            elif(_line[0:2] == "K " ):
                #Manufacturer tool,Kommentar
                leer = 0
            elif(_line[0:2] == "D " ):
                #Date of file
                leer = 0
            elif(_line[0:2] == "V " ):
                #Version
                leer = 0
            elif(_line[0:2] == "H " ):
                #Typ of Device
                leer = 0
            elif(_line[0:1] == "-" ):

                #Blockende

                self.read_caption=False    #C-Block
                self.read_record=False     #R-Block
                self.read_table=False      #T-Block
                self.read_value=False      #Values


            #start with "T " followed by a number -> new table
            elif(_line[0:2] == "T " and _line[2:3].isdigit() == True):
                linedata = _line.split()

                try:

                    #extract Table-Number
                    parseInt = int(linedata[1])
                    #delete Caption-Array
                    del self.captions[0:len(self.captions)]
                    #found new Table
                    self.read_table=True


                except TypeError:
                    # not a number
                    self.read_value=True
                    self.read_record=False


            #start with "C " followed by a number -> new Caption
            elif(_line[0:1] == "C" and _line[1:2].isdigit() == True):
                if(self.read_table == True and self.read_caption == False):
                    self.read_caption=True

            #start with "R " followed by a number -> new Record
            elif(_line[0:2] == "R " and _line[2:3].isdigit() == True):

                self.read_value=False
                self.read_caption=False
                self.read_record=True
                self.read_table=False


                self.__ValueCount = len(self.captions)
                self.val_count = 0


            #last entry
            elif(_line[0:3] == "XXX"):

                _OutFileObj.write("\t</"+self.recordname+">")
                _OutFileObj.write("</eib-products>")
                _OutFileObj.flush()

                return


            #else: read Values
            else:
                self.read_value=True
                self.read_record=False


            #Do we read a table ?
            if(self.read_table == True):
                tunix=0

            #Do we read a Record ?
            if(self.read_record == True):

                self.atmp=_line.split(" ")

                self.val_count=0

                if(self.recordname != ""):
                    #record end
                    _OutFileObj.write("\t</"+self.recordname+">\n")

                #z.Bsp: R 1 T 3 manufacturer
                self.recordname=self.atmp[4]
                _OutFileObj.write("\t<"+self.recordname+">\n")


            #Do we read a caption ?
            if(self.read_caption == True):
                self.atmp=_line.split(" ")
                #add new caption-element
                self.captions.append(self.atmp[5])

            #Do we read a Value ?
            if(self.read_value == True and self.val_count <= len(self.captions)-1):

              #  if(self.captions[self.val_count] == "EEPROM_DATA" or self.captions[self.val_count] == "DATA_LENGTH"):
              #      print _line
              #      print self.CurLine
              #      print No

                if(_line != ""):
                    _line = _line.replace("<","&lt;")
                    _line = _line.replace(">","&gt;")
                    _line = _line.replace("&", "&amp;")
                    #write Values to file
                    _OutFileObj.write("\t\t<"+self.captions[self.val_count]+">"+_line+"</"+self.captions[self.val_count]+">\n")

                    #LOG File
                   #     self.__LogObj.NewLog("\t\t<"+self.captions[self.val_count]+">"+_line+"</"+self.captions[self.val_count]+">\n",0)


                else:
                    self.__ValueCount = 0
                    self.read_value == False

                self.val_count = self.val_count + 1


            _line=""


        except(IOError):
            print "Fehler"




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

