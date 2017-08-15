#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_InstallationDataModel.py
#Version: V0.1 , 12.05.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================


from xml.dom import minidom
from xml.dom.minidom import *
from XML.FB_XMLDataModel import FB_XMLDataModel #import class as base class
import os
import cPickle


##general class for handling project data which are based on XML
class FB_InstallationDataModel(FB_XMLDataModel):


    __LogObj = None
    __DOMObj = None
    __DEVICES_ROOT_ID = "InstallationDevices-1"
    __instDocument = None
    __PATH = ""         #Project Path
    __sensorsNode = None
    __actuatorsNode = None
    __passiveNode = None

    ##Constructor
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param projectname: Path and name of project
    def __init__(self, LogObj, InstDocument,projectname, new):
        FB_XMLDataModel.__init__(self,LogObj,InstDocument,projectname)

        self.__LogObj = LogObj
        self.__instDocument = InstDocument

        #archNode = Document.appendChild(Document.createElement("architectural-data"))
        #find mainnode "installation-components"
        if(new == True):
            instNode = self.__instDocument.documentElement

            activeNode = instNode.appendChild(self.__instDocument.createElement("InstallationDevices"))
            activeNode.setAttribute("id", self.__DEVICES_ROOT_ID)

            #complete installation structure:
            #Devices -> id = InstallationDevices-1
            #    Produktname id = Device-1
            #        Gruppenadressen
            #        all programming parameters
            #        all parameters according to this product (for reconstruction without the database)

            OutFileObj = open("installation.xml","w")
            String = self.__instDocument.toxml(encoding = "ISO-8859-1")
            OutFileObj.write(String)
            OutFileObj.close()

    ##adds a new device with the given ID (device-1) and creates all subnodes to store additional data of the product
    def AddDevice(self,ID):
        #get parent node
        parent = self.getDataRootNode(self.__DEVICES_ROOT_ID)

        Element = self.__instDocument.createElement("Device")
        Element.setAttribute("id", ID)

        Element.appendChild(self.__instDocument.createElement("DeviceInstance"))
        #more childs....

        #and bind it...
        parent.appendChild(Element)


    #save a instance of the device-class
    def WriteDeviceInstance(self,ID, Instance):
        #write instance to section 'DeviceInstance'
        #get root-node of ID
        parentNode = self.getDataRootNode(ID)
        #prepare Instance
        DumpStream = cPickle.dumps(Instance,2)
        #self.writeDOMNodeValue(parentNode,"DeviceInstance", unicode(DumpStream,"ISO-8859-1"))

#****************************************************************************
    def setProjectPath(self,projectPath):
        self.__PATH = projectPath

    def getProjectPath(self):
        return self.__PATH
#****************************************************************************


#****************************************************************************

    def SaveInstmodel(self):
        os.chdir(self.getProjectPath())
        OutFileObj = open("installation.xml","w")
        String = self.__instDocument.toxml(encoding = "ISO-8859-1")
        OutFileObj.write(String)
        OutFileObj.close()
