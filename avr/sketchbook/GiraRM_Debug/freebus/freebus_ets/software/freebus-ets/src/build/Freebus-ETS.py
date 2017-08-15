#-*- coding: iso-8859-1 -*-
#!/usr/bin/
#===============================================================================
#Source File: Testaufruf.py
#Version: V0.1 vom 25.12.2007
#Autor: Jerome Leisner
#===============================================================================

#Path-structure
#Installation-Path
#    ->    Dist (name of directory of setup procedure
#    ->    Logging
#    ->    src
#            -> GUI
#            -> Image

import os
import sys

from XML import FB_XMLConverter
from XML import FB_XML_PRODUCT
from XML import FB_XMLDataModel
from LOG import Logging
from GUI import FB_MainFrame


#add Directory Structure
#GUI Path
GUIPath = os.getcwd() + "\\GUI\\"
LogPath = "../Logging/"

print GUIPath

#List of Prefixes for Project-Structure
Prefix = [" ", "Project","Building","Floor", "Room", "Junction"]

LogFileName = LogPath + 'Log.log' #"J:/Elektronik/Projekte/EIB/ETS for Freebus/Log/XMLLog.log"
Options = 0

#LOG_XML = Logging.Logging("XML_Converter",LogFileName,Options)



LOG_XML = Logging.Logging("FB_XML",LogFileName,Options)

#file = "J:/Elektronik/Projekte/EIB/ETS for Freebus/SunXML.xml"

#projectfile = "J:/Elektronik/Projekte/EIB/ETS for Freebus/Test.xml"


#XML = FB_XMLConverter.FB_XMLConverter("J:/Elektronik/Projekte/EIB/ETS for Freebus/CSuntracer2.vd_", \
#                                  file, LOG_XML)


FBMain = FB_MainFrame.FB_MainFrame(LOG_XML)
FBMain.main()

#******* CONVERT ***************

#XML.convertToXML()

#******* PROJECT *********************************
#Project.setProjectName("Pooopel")
#Project.setPrefferedBusSystem("LON")
#Project.setComment("is doch alles klar...ääh?")

#print Project.getProjectName()
#print Project.getComment()
#print Project.getPrefferedBusSystem()
#print Project.getProjectDirectoryName()

#Project.SaveProject()


#FB_XML_new = FB_XML_PRODUCT.FB_XML_PRODUCT(LOG_XML, newfile)
FB_XML = FB_XML_PRODUCT.FB_XML_PRODUCT(LOG_XML, file)
#new product-file
#FB_XML_new.CreateProductFile() #create also DOM-Object

#productList = FB_XML.getProducts(file)

#AppList = FB_XML.getApplications(file)

#create parse-object and parse xml-file -> all sub-handler will be called automatically

#XMLHandler = FB_XML.parseXMLFile() #call also after creation of file
XMLHandler = -1
if(XMLHandler != -1):
    #Product-List
    productList = FB_XML.getProducts(XMLHandler)
    #Applications/Manufacturer-List
    AppList = FB_XML.getApplications(XMLHandler)
    #communicatiuon object List
    CommObjList = FB_XML.getCommunicationObjects(XMLHandler)
    #prod2programm object List
    Prod2Progr = FB_XML.getProd2Progr(XMLHandler)
    #mask object list
    Mask = FB_XML.getMask(XMLHandler)

    productList[0].setProductName("Freebus")
    CommObjList[0].setObjName("Schalt Dings Bums äääh")
    Mask[0].setMaskVersion("4.05")

    FB_XML.SaveDocument()
#    print AppManList[0][0].getProgramName()
 #   print AppManList[1][0].getManufactName()

 #   print CommObjList[0].getProgramID()
 #   print Prod2Progr[0].getProductID()

#    print Mask[0].getMaskID()
#    print Mask[0].getMaskVersion()
#    print Mask[0].getUserRamStart()
 #   print Mask[0].getUserRamEnd()
#    print Mask[0].getUserEEpromStart()
#    print Mask[0].getUserEEpromEnd()
#    print Mask[0].getRunErrorAddr()
#    print Mask[0].getAddrTabAddr()
#    print Mask[0].getAssocTabAddr()
#    print Mask[0].getCommsTabAddr()
 #   print Mask[0].getManufactDataAddr()
 #   print Mask[0].getManufactDataSize()
 #   print Mask[0].getManufactIDAddr()
 #   print Mask[0].getRoutAddr()
 #   print Mask[0].getManufactIDProtect()
 #   print Mask[0].getMaskVersionName()
 #   print Mask[0].getMaskDataLength()
 #   print Mask[0].getAddressTab()
 #   print Mask[0].getAssocTab()
 #   print Mask[0].getAppProgram()
 #   print Mask[0].getPEIProgram()
 #   print Mask[0].getLoadControlAddr()
 #   print Mask[0].getRunControlAddr()
 #   print Mask[0].getPortAddrProtected()
 #   print Mask[0].getMediumTypeNo()
 #   print Mask[0].getBCUTypeNo()


  #  print len(CommObjList)
else:
    print "geht nicht"

#ende =  len(AppList[0])
#print ende
#for i in range(ende):
#    print AppList[0][i].getProgramID()
#    print AppList[0][i].getSymbolID()
#    print AppList[0][i].getMaskID()
#    print AppList[0][i].getProgramName()
#    print AppList[0][i].getProgramV()
#    print AppList[0][i].getProgramVNo()
#    print AppList[0][i].getLinkable()
 #   print AppList[0][i].getDeviceType()
 #   print AppList[0][i].getPEIType()
#    print AppList[0][i].getAddrTabSize()
#    print AppList[0][i].getAssTabAddr()
 #   print AppList[0][i].getComTabAddr()
#    print AppList[0][i].getComTabSize()
#    print AppList[0][i].getProgramSN()

#    print AppList[0][i].getEEPRPOMData()
#    print AppList[0][i].getDataLength()
#    print AppList[0][i].getS19File()
#    print AppList[0][i].getMapFile()
#    print AppList[0][i].getAssID()
#    print AppList[0][i].getHelpFileName()
#    print AppList[0][i].getContextID()
#    print AppList[0][i].getDynMng()

#    print AppList[0][i].getRamSize()
#    print AppList[0][i].getOrigManID()
#    print AppList[0][i].getAPIVersion()
 #   print AppList[0][i].getProgramStyle()
#    print AppList[0][i].getPollingMaster()
#    print AppList[0][i].getPollingGroups()
#    print AppList[0][i].getAllowETS()
#    print AppList[0][i].getMinETS()


#ende =  len(AppList[1])
#for i in range(ende):
#    print AppList[1][i].getManufactName()
#    print AppList[1][i].getManufactID()

#ende = len(productList)

#for i in range(ende):
#    print productList[i].getProductID()
#    print productList[i].getManufactuerID()
#    print productList[i].getSymbolID()
#    print productList[i].getProductName()
#    print productList[i].getProductVersion()
#    print productList[i].getCompType()
#    print productList[i].getCompAttr()
#    print productList[i].getBusCurrent()
#    print productList[i].getProductSN()
 #   print productList[i].getCompTypeNo()
 #   print productList[i].getProductPic()
#    print productList[i].getBCUType()
#    print productList[i].getProductHandling()
#    print productList[i].getProductDLL()
 #   print productList[i].getOrigManID()


#  print CommObjList[0].getObjName()
#    print CommObjList[0].getObjFunction()
#    print CommObjList[0].getObjReadEN()
#    print CommObjList[0].getObjWriteEN()
#    print CommObjList[0].getObjCommEN()
#    print CommObjList[0].getObjTransEN()
#    print CommObjList[0].getObjDisplOrder()
#    print CommObjList[0].getParentParaValue()
#    print CommObjList[0].getObjID()
#    print CommObjList[0].getParaID()
#    print CommObjList[0].getObjNumber()
#    print CommObjList[0].getObjType()
#    print CommObjList[0].getObjPriority()
#    print CommObjList[0].getObjUpdateEN()
#    print CommObjList[0].getObjUniqueNo()
