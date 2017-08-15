#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source Datei: FB_AppXMLHandler.py
#Version: V0.1 , 25.12.2007
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#
#===============================================================================


from FB_DATA import FB_Applications
from FB_DATA import FB_Manufacturer

##Class for Extracting Applications from FB-XML Productfiles
class FB_AppXMLHandler():

    __LogObj = None
    __isApp = False          #Application found ?
    __isMan = False          #Manufacturer found ?
    __isProgID = False

    __App = None
    __Applications = []      #List of Instances with Apllications (FB_Applications)
    __Man = None
    __Manufacturer = []      #List of Instances with Manufacturer (FB_Manufacturer)

    #Sub-Nodes of application_program
    __isProgID = False       #PROGRAM_ID
    __isSymbolID = False     #SYMBOL_ID
    __isMaskID = False       #MASK_ID
    __isProgName = False     #PROGRAM_NAME
    __isProgVersion = False  #PROGRAM_VERSION
    __isProgVersionNo = False#PROGRAM_VERSION_NUMBER
    __isLinkable = False     #LINKABLE
    __isDevType = False      #DEVICE_TYPE
    __isPEIType = False      #PEI_TYPE
    __isAddTabsize = False   #ADDRESS_TAB_SIZE
    __isAssTabAddr = False   #ASSOCTAB_ADDRESS
    __isAssTsize = False     #ASSOCTAB_SIZE
    __isComTabAddr = False   #COMMSTAB_ADDRESS
    __isComTabsize = False   #COMMSTAB_SIZE
    __isProgSN = False       #PROGRAM_SERIAL_NUMBER
    __isAppManID = False     #MANUFACTURER_ID
    __isEEPROMData = False   #EEPROM_DATA
    __isDataLength = False   #DATA_LENGTH
    __isS19File = False      #S19_FILE
    __isMapFile = False      #MAP_FILE
    __isAssemblerID = False  #ASSEMBLER_ID
    __isHelpFileName = False #HELP_FILE_NAME
    __isContextID = False    #CONTEXT_ID
    __isDynamicMng = False   #DYNAMIC_MANAGEMENT
    __isProgType = False     #PROGRAM_TYPE
    __isRAMSize = False      #RAM_SIZE
    __isOrgManID = False     #ORIGINAL_MANUFACTURER_ID
    __isAPIVersion = False   #API_VERSION
    __isProgStyle = False    #PROGRAM_STYLE
    __isPollMaster = False   #IS_POLLING_MASTER
    __isNoPollGroups = False #NUMBER_OF_POLLING_GROUPS
    __isAllowedInETS = False #AllowedInSimpleEts
    __isMinEtsVersion = False#MinEtsVersion


    #Sub-Nodes of manufacturer
    __isManID = False        #MANUFACTURER_ID
    __isManName = False      #MANUFACTURER_NAME

    #Constructor for FB_AppXMLHandler.
    def __init__(self, LogObj):
        self.__LogObj = LogObj
        self.__App = FB_Applications.FB_Apps()
        self.__Man = FB_Manufacturer.FB_Manufacturer()
        #self.__LogObj.NewLog("erzeugt: " ,0)

    #return List of Instances of type FB_Applications
    def getAppList(self):
         return self.__Applications

    #return List of Instances of type FB_Manufacturer
    def getManufacturerList(self):
        return self.__Manufacturer

    def startDocument(self):
        try:
            #self.__LogObj.NewLog("startDocument: " ,0)
            pass

        except SAXException:
            pass

    def endDocument(self):
        try:
            #self.__LogObj.NewLog("endDocument: " ,0)
            pass

        except SAXException:
            pass

    def startElement(self, eName, attrs):

        try:
        #Look at SubNodes manufacturer
            if(eName == "manufacturer"):
                self.__isMan = True
                #print "Manufact Start"

            elif(eName == "MANUFACTURER_NAME"):
                if(self.__isMan == True):
                    self.__isManName = True
                    #print "Manu Name Start"

            elif(eName == "MANUFACTURER_ID" ):
                if(self.__isMan == True):
                    self.__isManID = True
                if(self.__isApp == True):
                    self.__isAppManID = True

        #Look at SubNodes application_program
            elif(eName == "application_program"):
                self.__isApp = True


            elif(eName == "PROGRAM_ID"):
                if(self.__isApp == True):
                    self.__isProgID = True

            elif(eName == "SYMBOL_ID"):
                if(self.__isApp == True):
                    self.__isSymbolID = True

            elif(eName == "MASK_ID"):
                if(self.__isApp == True):
                    self.__isMaskID = True

            elif(eName == "PROGRAM_NAME"):
                if(self.__isApp == True):
                    self.__isProgName = True

            elif(eName == "PROGRAM_VERSION"):
                if(self.__isApp == True):
                    self.__isProgVersion = True

            elif(eName == "PROGRAM_VERSION_NUMBER"):
                if(self.__isApp == True):
                    self.__isProgVersionNo = True

            elif(eName == "LINKABLE"):
                if(self.__isApp == True):
                    self.__isLinkable = True

            elif(eName == "DEVICE_TYPE"):
                if(self.__isApp == True):
                    self.__isDevType = True

            elif(eName == "PEI_TYPE"):
                if(self.__isApp == True):
                    self.__isPEIType = True

            elif(eName == "ADDRESS_TAB_SIZE"):
                if(self.__isApp == True):
                    self.__isAddTabsize = True

            elif(eName == "ASSOCTAB_ADDRESS"):
                if(self.__isApp == True):
                    self.__isAssTabAddr = True

            elif(eName == "ASSOCTAB_SIZE"):
                if(self.__isApp == True):
                    self.__isAssTsize = True

            elif(eName == "COMMSTAB_ADDRESS"):
                if(self.__isApp == True):
                    self.__isComTabAddr = True

            elif(eName == "COMMSTAB_SIZE"):
                if(self.__isApp == True):
                    self.__isComTabsize = True

            elif(eName == "PROGRAM_SERIAL_NUMBER"):
                if(self.__isApp == True):
                    self.__isProgSN = True

            elif(eName == "EEPROM_DATA"):
                if(self.__isApp == True):
                    self.__isEEPROMData = True

            elif(eName == "DATA_LENGTH"):
                if(self.__isApp == True):
                    self.__isDataLength = True

            elif(eName == "S19_FILE"):
                if(self.__isApp == True):
                    self.__isS19File = True

            elif(eName == "MAP_FILE"):
                if(self.__isApp == True):
                    self.__isMapFile = True

            elif(eName == "ASSEMBLER_ID"):
                if(self.__isApp == True):
                    self.__isAssemblerID = True

            elif(eName == "HELP_FILE_NAME"):
                if(self.__isApp == True):
                    self.__isHelpFileName = True

            elif(eName == "CONTEXT_ID"):
                if(self.__isApp == True):
                    self.__isContextID = True

            elif(eName == "DYNAMIC_MANAGEMENT"):
                if(self.__isApp == True):
                    self.__isDynamicMng = True

            elif(eName == "PROGRAM_TYPE"):
                if(self.__isApp == True):
                    self.__isProgType = True

            elif(eName == "RAM_SIZE"):
                if(self.__isApp == True):
                    self.__isRAMSize = True

            elif(eName == "ORIGINAL_MANUFACTURER_ID"):
                if(self.__isApp == True):
                    self.__isOrgManID = True

            elif(eName == "API_VERSION"):
                if(self.__isApp == True):
                    self.__isAPIVersion = True

            elif(eName == "PROGRAM_STYLE"):
                if(self.__isApp == True):
                    self.__isProgStyle = True

            elif(eName == "IS_POLLING_MASTER"):
                if(self.__isApp == True):
                    self.__isPollMaster = True

            elif(eName == "NUMBER_OF_POLLING_GROUPS"):
                if(self.__isApp == True):
                    self.__isNoPollGroups = True

            elif(eName == "AllowedInSimpleEts"):
                if(self.__isApp == True):
                    self.__isAllowedInETS = True

            elif(eName == "MinEtsVersion"):
                if(self.__isApp == True):
                    self.__isMinEtsVersion = True


        except SAXException:
            print "Error again"

    def endElement(self,eName):

        try:

            #self.__LogObj.NewLog("endElement: " ,0)
            if(eName == "manufacturer"):
                self.__isMan=False
                #add Instanz to the list
                self.__Manufacturer.append(self.__Man)
                self.__Man = None
                self.__Man = FB_Manufacturer.FB_Manufacturer()
                #print "Manufact Ende"

            elif(eName == "application_program"):
                self.__isApp=False

                #add Instanz to the list
                self.__Applications.append(self.__App)
                self.__App = None
                self.__App = FB_Applications.FB_Apps()

            #Look at SubNodes of manufacturer
            elif(eName == "MANUFACTURER_NAME"):
                if(self.__isMan == True):
                    self.__isManName=False
                    #print "Manufact Name Ende"

            elif(eName == "MANUFACTURER_ID"):
                if(self.__isMan == True):
                    self.__isManID = False
                if(self.__isApp == True):
                    self.__isAppManID = False


            #Look at SubNodes of application_program
            elif(eName == "PROGRAM_ID"):
                if(self.__isApp == True):
                    self.__isProgID = False

            elif(eName == "SYMBOL_ID"):
                if(self.__isApp == True):
                    self.__isSymbolID = False

            elif(eName == "MASK_ID"):
                if(self.__isApp == True):
                    self.__isMaskID = False

            elif(eName == "PROGRAM_NAME"):
                if(self.__isApp == True):
                    self.__isProgName = False

            elif(eName == "PROGRAM_VERSION"):
               if(self.__isApp == True):
                    self.__isProgVersion = False

            elif(eName == "PROGRAM_VERSION_NUMBER"):
                if(self.__isApp == True):
                    self.__isProgVersionNo = False

            elif(eName == "LINKABLE"):
                if(self.__isApp == True):
                    self.__isLinkable = False

            elif(eName == "DEVICE_TYPE"):
                if(self.__isApp == True):
                    self.__isDevType = False

            elif(eName == "PEI_TYPE"):
                if(self.__isApp == True):
                    self.__isPEIType = False

            elif(eName == "ADDRESS_TAB_SIZE"):
                if(self.__isApp == True):
                    self.__isAddTabsize = False

            elif(eName == "ASSOCTAB_ADDRESS"):
                if(self.__isApp == True):
                    self.__isAssTabAddr = False

            elif(eName == "ASSOCTAB_SIZE"):
                if(self.__isApp == True):
                    self.__isAssTsize = False

            elif(eName == "COMMSTAB_ADDRESS"):
                if(self.__isApp == True):
                    self.__isComTabAddr = False

            elif(eName == "COMMSTAB_SIZE"):
                if(self.__isApp == True):
                    self.__isComTabsize = False

            elif(eName == "PROGRAM_SERIAL_NUMBER"):
                if(self.__isApp == True):
                    self.__isProgSN = False

            elif(eName == "EEPROM_DATA"):
                if(self.__isApp == True):
                    self.__isEEPROMData = False

            elif(eName == "DATA_LENGTH"):
                if(self.__isApp == True):
                    self.__isDataLength = False

            elif(eName == "S19_FILE"):
                if(self.__isApp == True):
                    self.__isS19File = False

            elif(eName == "MAP_FILE"):
                if(self.__isApp == True):
                    self.__isMapFile = False

            elif(eName == "ASSEMBLER_ID"):
                if(self.__isApp == True):
                    self.__isAssemblerID = False

            elif(eName == "HELP_FILE_NAME"):
                if(self.__isApp == True):
                    self.__isHelpFileName = False

            elif(eName == "CONTEXT_ID"):
                if(self.__isApp == True):
                    self.__isContextID = False

            elif(eName == "DYNAMIC_MANAGEMENT"):
                if(self.__isApp == True):
                    self.__isDynamicMng = False

            elif(eName == "PROGRAM_TYPE"):
                if(self.__isApp == True):
                    self.__isProgType = False

            elif(eName == "RAM_SIZE"):
                if(self.__isApp == True):
                    self.__isRAMSize = False

            elif(eName == "ORIGINAL_MANUFACTURER_ID"):
                if(self.__isApp == True):
                    self.__isOrgManID = False

            elif(eName == "API_VERSION"):
                if(self.__isApp == True):
                    self.__isAPIVersion = False

            elif(eName == "PROGRAM_STYLE"):
                if(self.__isApp == True):
                    self.__isProgStyle = False

            elif(eName == "IS_POLLING_MASTER"):
                if(self.__isApp == True):
                    self.__isPollMaster = False

            elif(eName == "NUMBER_OF_POLLING_GROUPS"):
                if(self.__isApp == True):
                    self.__isNoPollGroups = False

            elif(eName == "AllowedInSimpleEts"):
                if(self.__isApp == True):
                    self.__isAllowedInETS = False

            elif(eName == "MinEtsVersion"):
                if(self.__isApp == True):
                    self.__isMinEtsVersion = False

        except SAXException:
            print "Error again"

    def characters(self ,char):
        #print char
 #       self.__LogObj.NewLog("char: " + char.encode( "iso-8859-1" ) ,0)

        if(self.__isManName == True):
            self.__Man.setManufactName(char.encode( "iso-8859-1" ))

        if(self.__isManID == True):
            self.__Man.setManufactID(char.encode( "iso-8859-1" ))

        if(self.__isProgID == True):
            self.__App.setProgramID(char.encode( "iso-8859-1" ))

        if(self.__isSymbolID == True):
            self.__App.setSymbolID(char.encode( "iso-8859-1" ))

        if(self.__isMaskID == True):
            self.__App.setMaskID(char.encode( "iso-8859-1" ))

        if(self.__isProgName == True):
            self.__App.setProgramName(char.encode( "iso-8859-1" ))

        if(self.__isProgVersion == True):
            self.__App.setProgramV(char.encode( "iso-8859-1" ))

        if(self.__isProgVersionNo == True):
            self.__App.setProgramVNo(char.encode( "iso-8859-1" ))

        if(self.__isLinkable == True):
            self.__App.setLinkable(char.encode( "iso-8859-1" ))

        if(self.__isDevType == True):
            self.__App.setDeviceType(char.encode( "iso-8859-1" ))

        if(self.__isPEIType == True):
            self.__App.setPEIType(char.encode( "iso-8859-1" ))

        if(self.__isAddTabsize == True):
            self.__App.setAddrTabSize(char.encode( "iso-8859-1" ))

        if(self.__isAssTabAddr == True):
            self.__App.setAssTabAddr(char.encode( "iso-8859-1" ))

        if(self.__isAssTsize == True):
            self.__App.setAssTabSize(char.encode( "iso-8859-1" ))

        if(self.__isComTabAddr == True):
            self.__App.setComTabAddr(char.encode( "iso-8859-1" ))

        if(self.__isComTabsize == True):
            self.__App.setComTabSize(char.encode( "iso-8859-1" ))

        if(self.__isProgSN == True):
            self.__App.setProgramSN(char.encode( "iso-8859-1" ))

        if(self.__isAppManID == True):
            self.__App.setAppManufacID(char.encode( "iso-8859-1" ))

        if(self.__isEEPROMData == True):
            self.__App.setEEPRPOMData(char.encode( "iso-8859-1" ))

        if(self.__isDataLength == True):
            self.__App.setDataLength(char.encode( "iso-8859-1" ))

        if(self.__isS19File == True):
            self.__App.setS19File(char.encode( "iso-8859-1" ))

        if(self.__isMapFile == True):
            self.__App.setMapFile(char.encode( "iso-8859-1" ))

        if(self.__isAssemblerID == True):
            self.__App.setAssID(char.encode( "iso-8859-1" ))

        if(self.__isHelpFileName == True):
            self.__App.setHelpFileName(char.encode( "iso-8859-1" ))

        if(self.__isContextID == True):
            self.__App.setContextID(char.encode( "iso-8859-1" ))

        if(self.__isDynamicMng == True):
            self.__App.setDynMng(char.encode( "iso-8859-1" ))

        if(self.__isProgType == True):
            self.__App.setProgramType(char.encode( "iso-8859-1" ))

        if(self.__isRAMSize == True):
            self.__App.setRamSize(char.encode( "iso-8859-1" ))

        if(self.__isOrgManID == True):
            self.__App.setOrigManID(char.encode( "iso-8859-1" ))

        if(self.__isAPIVersion == True):
            self.__App.setAPIVersion(char.encode( "iso-8859-1" ))

        if(self.__isProgStyle == True):
            self.__App.setProgramStyle(char.encode( "iso-8859-1" ))

        if(self.__isPollMaster == True):
            self.__App.setPollingMaster(char.encode( "iso-8859-1" ))

        if(self.__isNoPollGroups == True):
            self.__App.setPollingGroups(char.encode( "iso-8859-1" ))

        if(self.__isAllowedInETS == True):
            self.__App.setAllowETS(char.encode( "iso-8859-1" ))

        if(self.__isMinEtsVersion == True):
            self.__App.setMinETS(char.encode( "iso-8859-1" ))

