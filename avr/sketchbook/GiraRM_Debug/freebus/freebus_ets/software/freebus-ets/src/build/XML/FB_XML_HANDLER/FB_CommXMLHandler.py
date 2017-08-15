#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source Datei: FB_CommXMLHandler.py
#Version: V0.1 , 31.12.2007
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#
#===============================================================================

from FB_DATA import FB_CommunicationObj


##Class for Extracting Communication-objects from FB-XML Productfiles
class FB_CommXMLHandler():

    __LogObj = None
    __isCommObj = False          #Communication-object found ?
    __CommObj = None             #object of type FB_CommunicationObj
    __CommunicationObjects = []  #List of Instances with communication-objects

    __ProgID = False           #PROGRAM_ID
    __ObjName = False          #OBJECT_NAME
    __ObjFunction = False      #OBJECT_FUNCTION
    __ObjReadEnabled = False   #OBJECT_READENABLED
    __ObjWriteEnabled = False  #OBJECT_WRITEENABLED
    __ObjCommEnabled = False   #OBJECT_COMMENABLED
    __ObjTransEnabled = False  #OBJECT_TRANSENABLED
    __ObjDisplayOrder = False  #OBJECT_DISPLAY_ORDER
    __ParentParamValue = False #PARENT_PARAMETER_VALUE
    __ObjID = False            #OBJECT_ID
    __ParamID = False          #PARAMETER_ID
    __ObjNumber = False        #OBJECT_NUMBER
    __ObjType = False          #OBJECT_TYPE
    __ObjPriority = False      #OBJECT_PRIORITY
    __ObjUpdateEnabled = False #OBJECT_UPDATEENABLED
    __ObjUniqueNumber = False  #OBJECT_UNIQUE_NUMBER


    #Constructor for FB_CommXMLHandler.
    def __init__(self, LogObj):
        self.__LogObj = LogObj
        self.__CommObj = FB_CommunicationObj.FB_CommObj()


    #return List of Instances of type FB_CommunicationsObj
    def getCommObjList(self):
         return self.__CommunicationObjects

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
            #self.__LogObj.NewLog("startElement: " ,0)
        #Look at SubNodes communication objects
            if(eName == "communication_object"):
                self.__isCommObj = True

            elif(eName == "PROGRAM_ID"):
                if(self.__isCommObj == True):
                    self.__ProgID = True

            elif(eName == "OBJECT_NAME"):
                if(self.__isCommObj == True):
                    self.__ObjName = True

            elif(eName == "OBJECT_FUNCTION"):
                if(self.__isCommObj == True):
                    self.__ObjFunction = True

            elif(eName == "OBJECT_READENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjReadEnabled = True

            elif(eName == "OBJECT_WRITEENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjWriteEnabled = True

            elif(eName == "OBJECT_COMMENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjCommEnabled = True

            elif(eName == "OBJECT_TRANSENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjTransEnabled = True

            elif(eName == "OBJECT_DISPLAY_ORDER"):
                if(self.__isCommObj == True):
                    self.__ObjDisplayOrder = True

            elif(eName == "PARENT_PARAMETER_VALUE"):
                if(self.__isCommObj == True):
                    self.__ParentParamValue = True

            elif(eName == "OBJECT_ID"):
                if(self.__isCommObj == True):
                    self.__ObjID = True

            elif(eName == "PARAMETER_ID"):
                if(self.__isCommObj == True):
                    self.__ParamID = True

            elif(eName == "OBJECT_NUMBER"):
                if(self.__isCommObj == True):
                    self.__ObjNumber = True

            elif(eName == "OBJECT_TYPE"):
                if(self.__isCommObj == True):
                    self.__ObjType = True

            elif(eName == "OBJECT_PRIORITY"):
                if(self.__isCommObj == True):
                    self.__ObjPriority = True

            elif(eName == "OBJECT_UPDATEENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjUpdateEnabled = True

            elif(eName == "OBJECT_UNIQUE_NUMBER"):
                if(self.__isCommObj == True):
                    self.__ObjUniqueNumber = True


        except SAXException:
            print "Error again"

    def endElement(self,eName):

        try:
             #self.__LogObj.NewLog("endElement: " ,0)
            if(eName == "communication_object"):
                self.__isCommObj=False
                #add Instanz to the list
                self.__CommunicationObjects.append(self.__CommObj)
                self.__CommObj = None
                self.__CommObj = FB_CommunicationObj.FB_CommObj()

            elif(eName == "PROGRAM_ID"):
                if(self.__isCommObj == True):
                    self.__ProgID = False

            elif(eName == "OBJECT_NAME"):
                if(self.__isCommObj == True):
                    self.__ObjName = False

            elif(eName == "OBJECT_FUNCTION"):
                if(self.__isCommObj == True):
                    self.__ObjFunction = False

            elif(eName == "OBJECT_READENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjReadEnabled = False

            elif(eName == "OBJECT_WRITEENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjWriteEnabled = False

            elif(eName == "OBJECT_COMMENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjCommEnabled = False

            elif(eName == "OBJECT_TRANSENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjTransEnabled = False

            elif(eName == "OBJECT_DISPLAY_ORDER"):
                if(self.__isCommObj == True):
                    self.__ObjDisplayOrder = False

            elif(eName == "PARENT_PARAMETER_VALUE"):
                if(self.__isCommObj == True):
                    self.__ParentParamValue = False

            elif(eName == "OBJECT_ID"):
                if(self.__isCommObj == True):
                    self.__ObjID = False

            elif(eName == "PARAMETER_ID"):
                if(self.__isCommObj == True):
                    self.__ParamID = False

            elif(eName == "OBJECT_NUMBER"):
                if(self.__isCommObj == True):
                    self.__ObjNumber = False

            elif(eName == "OBJECT_TYPE"):
                if(self.__isCommObj == True):
                    self.__ObjType = False

            elif(eName == "OBJECT_PRIORITY"):
                if(self.__isCommObj == True):
                    self.__ObjPriority = False

            elif(eName == "OBJECT_UPDATEENABLED"):
                if(self.__isCommObj == True):
                    self.__ObjUpdateEnabled = False

            elif(eName == "OBJECT_UNIQUE_NUMBER"):
                if(self.__isCommObj == True):
                    self.__ObjUniqueNumber = False

        except SAXException:
            print "Error again"

    def characters(self ,char):
        #print char
 #       self.__LogObj.NewLog("char: " + char.encode( "iso-8859-1" ) ,0)
        if(self.__ProgID == True):
            self.__CommObj.setProgramID(char.encode( "iso-8859-1" ))

        if(self.__ObjName == True):
            self.__CommObj.setObjName(char.encode( "iso-8859-1" ))

        if(self.__ObjFunction == True):
            self.__CommObj.setObjFunction(char.encode( "iso-8859-1" ))

        if(self.__ObjReadEnabled == True):
            self.__CommObj.setObjReadEN(char.encode( "iso-8859-1" ))

        if(self.__ObjWriteEnabled == True):
            self.__CommObj.setObjWriteEN(char.encode( "iso-8859-1" ))

        if(self.__ObjCommEnabled == True):
            self.__CommObj.setObjCommEN(char.encode( "iso-8859-1" ))

        if(self.__ObjTransEnabled == True):
            self.__CommObj.setObjTransEN(char.encode( "iso-8859-1" ))

        if(self.__ObjDisplayOrder == True):
            self.__CommObj.setObjDisplOrder(char.encode( "iso-8859-1" ))

        if(self.__ParentParamValue == True):
            self.__CommObj.setParentParaValue(char.encode( "iso-8859-1" ))

        if(self.__ObjID == True):
            self.__CommObj.setObjID(char.encode( "iso-8859-1" ))

        if(self.__ParamID == True):
            self.__CommObj.setParaID(char.encode( "iso-8859-1" ))

        if(self.__ObjNumber == True):
            self.__CommObj.setObjNumber(char.encode( "iso-8859-1" ))

        if(self.__ObjType == True):
            self.__CommObj.setObjType(char.encode( "iso-8859-1" ))

        if(self.__ObjPriority == True):
            self.__CommObj.setObjPriority(char.encode( "iso-8859-1" ))

        if(self.__ObjUpdateEnabled == True):
            self.__CommObj.setObjUpdateEN(char.encode( "iso-8859-1" ))

        if(self.__ObjUniqueNumber == True):
            self.__CommObj.setObjUniqueNo(char.encode( "iso-8859-1" ))
