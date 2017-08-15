#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source Datei: FB_CommunicationObj.py
#Version: V0.1 , 31.12.2007
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

##Class for Handling of Freebus-Comm-Objects-Data which based from FB XML Product-Files
class FB_CommObj:

    __ProgID = ""           #PROGRAM_ID
    __ObjName = ""          #OBJECT_NAME
    __ObjFunction = ""      #OBJECT_FUNCTION
    __ObjReadEnabled = ""   #OBJECT_READENABLED
    __ObjWriteEnabled = ""  #OBJECT_WRITEENABLED
    __ObjCommEnabled = ""   #OBJECT_COMMENABLED
    __ObjTransEnabled = ""  #OBJECT_TRANSENABLED
    __ObjDisplayOrder = ""  #OBJECT_DISPLAY_ORDER
    __ParentParamValue = "" #PARENT_PARAMETER_VALUE
    __ObjID = ""            #OBJECT_ID
    __ParamID = ""          #PARAMETER_ID
    __ObjNumber = ""        #OBJECT_NUMBER
    __ObjType = ""          #OBJECT_TYPE
    __ObjPriority = ""      #OBJECT_PRIORITY
    __ObjUpdateEnabled = "" #OBJECT_UPDATEENABLED
    __ObjUniqueNumber = ""  #OBJECT_UNIQUE_NUMBER
    __return = None

    def __init__(self):
        #print "okk"
        pass

    def setCommObj(self,Index, Value):

        if(Index == 1):
            self.__ProgID = Value
        elif(Index == 2):
            self.__ObjName = Value
        elif(Index == 3):
            self.__ObjFunction = Value
        elif(Index == 4):
            self.__ObjReadEnabled = Value
        elif(Index == 5):
            self.__ObjWriteEnabled = Value
        elif(Index == 6):
            self.__ObjCommEnabled = Value
        elif(Index == 7):
            self.__ObjTransEnabled = Value
        elif(Index == 8):
            self.__ObjDisplayOrder = Value
        elif(Index == 9):
            self.__ParentParamValue = Value
        elif(Index == 10):
            self.__ObjID = Value
        elif(Index == 11):
            self.__ParamID = Value
        elif(Index == 12):
            self.__ObjNumber = Value
        elif(Index == 13):
            self.__ObjType = Value
        elif(Index == 14):
            self.__ObjPriority = Value
        elif(Index == 15):
            self.__ObjUpdateEnabled = Value
        elif(Index == 16):
            self.__ObjUniqueNumber = Value

    def getCommObj(self,Index):
        if(Index == 1):
            return self.__ProgID
        elif(Index == 2):
            return self.__ObjName
        elif(Index == 3):
            return self.__ObjFunction
        elif(Index == 4):
            return self.__ObjReadEnabled
        elif(Index == 5):
            return self.__ObjWriteEnabled
        elif(Index == 6):
            return self.__ObjCommEnabled
        elif(Index == 7):
            return self.__ObjTransEnabled
        elif(Index == 8):
            return self.__ObjDisplayOrder
        elif(Index == 9):
            return self.__ParentParamValue
        elif(Index == 10):
            return self.__ObjID
        elif(Index == 11):
            return self.__ParamID
        elif(Index == 12):
            return self.__ObjNumber
        elif(Index == 13):
            return self.__ObjType
        elif(Index == 14):
            return self.__ObjPriority
        elif(Index == 15):
            return self.__ObjUpdateEnabled
        elif(Index == 16):
            return self.__ObjUniqueNumber

#**********************************************************************
    #Handling PROGRAM_ID
    def setProgramID(self,P_ID):
        self.__ProgID = P_ID

    def getProgramID(self):
        return self.__ProgID
#**********************************************************************
    #Handling OBJECT_NAME
    def setObjName(self,O_Name):
        self.__ObjName = O_Name


    def getObjName(self):
        return self.__ObjName

#**********************************************************************
    #Handling OBJECT_FUNCTION
    def setObjFunction(self,ObjFct):
        self.__ObjFunction = ObjFct

    def getObjFunction(self):
        return self.__ObjFunction

#**********************************************************************
    #Handling OBJECT_READENABLED
    def setObjReadEN(self,R_EN):
        self.__ObjReadEnabled = R_EN

    def getObjReadEN(self):
        return self.__ObjReadEnabled

#**********************************************************************
    #Handling OBJECT_WRITEENABLED
    def setObjWriteEN(self,W_EN):
        self.__ObjWriteEnabled = W_EN

    def getObjWriteEN(self):
        return self.__ObjWriteEnabled

#**********************************************************************
    #Handling OBJECT_COMMENABLED
    def setObjCommEN(self,C_EN):
        self.__ObjCommEnabled = C_EN

    def getObjCommEN(self):
        return self.__ObjCommEnabled

#**********************************************************************
    #Handling OBJECT_TRANSENABLED
    def setObjTransEN(self,T_EN):
        self.__ObjTransEnabled = T_EN

    def getObjTransEN(self):
        return self.__ObjTransEnabled

#**********************************************************************
    # OBJECT_DISPLAY_ORDER
    def setObjDisplOrder(self,DisOrder):
        self.__ObjDisplayOrder = DisOrder

    def getObjDisplOrder(self):
        return self.__ObjDisplayOrder

#**********************************************************************
    # PARENT_PARAMETER_VALUE
    def setParentParaValue(self,PP_Value):
        self.__ParentParamValue = PP_Value

    def getParentParaValue(self):
        return self.__ParentParamValue

#**********************************************************************
    #OBJECT_ID
    def setObjID(self,O_ID):
        self.__ObjID = O_ID

    def getObjID(self):
        return self.__ObjID

#**********************************************************************
    #PARAMETER_ID
    def setParaID(self,P_ID):
        self.__ParamID = P_ID

    def getParaID(self):
        return self.__ParamID

#**********************************************************************
    #OBJECT_NUMBER
    def setObjNumber(self,O_No):
        self.__ObjNumber = O_No

    def getObjNumber(self):
        return self.__ObjNumber

#**********************************************************************
    #OBJECT_TYPE
    def setObjType(self,O_Type):
        self.__ObjType = O_Type

    def getObjType(self):
        return self.__ObjType

#**********************************************************************
    #OBJECT_PRIORITY
    def setObjPriority(self,O_Prio):
        self.__ObjPriority = O_Prio

    def getObjPriority(self):
        return self.__ObjPriority

#**********************************************************************
    #OBJECT_UPDATEENABLED
    def setObjUpdateEN(self,O_Up_EN):
        self.__ObjUpdateEnabled = O_Up_EN

    def getObjUpdateEN(self):
        return self.__ObjUpdateEnabled

#**********************************************************************
    #OBJECT_UNIQUE_NUMBER
    def setObjUniqueNo(self,O_UN_No):
        self.__ObjUniqueNumber = O_UN_No

    def getObjUniqueNo(self):
        return self.__ObjUniqueNumber

#**********************************************************************



