#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_Product.py
#Version: V0.1 , 16.11.2007
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

##Class for Handling of Freebus-Product-Data which based from FB XML Product-Files
class FB_Products:

    __Product_ID = ""         #PRODUCT_ID
    __Manufacturer_ID = ""    #MANUFACTURER_ID
    __SymbolID = ""           #SYMBOL_ID
    __Product_Name = ""       #PRODUCT_NAME
    __Product_Version = ""    #PRODUCT_VERSION_NUMBER
    __Comp_Type = ""          #COMPONENT_TYPE
    __Comp_Attr = ""          #COMPONENT_ATTRIBUTES
    __Bus_Current = ""        #BUS_CURRENT
    __Product_Serial = ""     #PRODUCT_SERIAL_NUMBER
    __CompTypeNo = ""         #COMPONENT_TYPE_NUMBER
    __ProductPic = ""         #PRODUCT_PICTURE
    __BCU_Type = ""           #BCU_TYPE_NUMBER
    __Product_Handling = ""   #PRODUCT_HANDLING
    __ProductDLL = ""         #PRODUCT_DLL
    __OrigManID = ""          #ORIGINAL_MANUFACTURER_ID


    def __init__(self):
        pass

    #general set and get property with index for each element, coresponds to FB_Constants
    def setProduct(self,Index, Value):
        if(Index == 1):
            self.__Product_ID = Value
        elif(Index == 2):
            self.__Manufacturer_ID = Value
        elif(Index == 3):
            self.__SymbolID = Value
        elif(Index == 4):
            self.__Product_Name = Value
        elif(Index == 5):
            self.__Product_Version = Value
        elif(Index == 6):
            self.__Comp_Type = Value
        elif(Index == 7):
            self.__Comp_Attr = Value
        elif(Index == 8):
            self.__Bus_Current = Value
        elif(Index == 9):
            self.__Product_Serial = Value
        elif(Index == 10):
            self.__CompTypeNo = Value
        elif(Index == 11):
            self.__ProductPic = Value
        elif(Index == 12):
            self.__BCU_Type = Value
        elif(Index == 13):
            self.__Product_Handling = Value
        elif(Index == 14):
            self.__ProductDLL = Value
        elif(Index == 15):
            self.__OrigManID = Value

    def getProduct(self,Index):
         if(Index == 1):
            return self.__Product_ID
         elif(Index == 2):
            return self.__Manufacturer_ID
         elif(Index == 3):
            return self.__SymbolID
         elif(Index == 4):
            return self.__Product_Name
         elif(Index == 5):
            return self.__Product_Version
         elif(Index == 6):
            return self.__Comp_Type
         elif(Index == 7):
            return self.__Comp_Attr
         elif(Index == 8):
            return self.__Bus_Current
         elif(Index == 9):
            return self.__Product_Serial
         elif(Index == 10):
            return self.__CompTypeNo
         elif(Index == 11):
            return self.__ProductPic
         elif(Index == 12):
            return self.__BCU_Type
         elif(Index == 13):
            return self.__Product_Handling
         elif(Index == 14):
            return self.__ProductDLL
         elif(Index == 15):
            return self.__OrigManID


#**********************************************************************
    #Handling Product ID
    def setProductID(self,P_ID):
        self.__Product_ID = P_ID

    def getProductID(self):
        return self.__Product_ID
#**********************************************************************
    #Handling Manufacturer_ID
    def setManufactuerID(self,M_ID):
        self.__Manufacturer_ID = M_ID

    def getManufactuerID(self):
        return self.__Manufacturer_ID

#**********************************************************************
    #Handling Symbol ID
    def setSymbolID(self,S_ID):
        self.__SymbolID = S_ID

    def getSymbolID(self):
        return self.__SymbolID

#**********************************************************************
    #Handling Product_Name
    def setProductName(self,P_Name):
        self.__Product_Name = P_Name

    def getProductName(self):
        return self.__Product_Name

#**********************************************************************

    #Handling Product_Version
    def setProductVersion(self,P_Version):
        self.__Product_Version = P_Version

    def getProductVersion(self):
        return self.__Product_Version

#**********************************************************************

    #Handling Component Type
    def setCompType(self,C_Type):
        self.__Comp_Type = C_Type

    def getCompType(self):
        return self.__Comp_Type

#**********************************************************************
    #Handling Component Attributes
    def setCompAttr(self,C_Attr):
        self.__Comp_Attr = C_Attr

    def getCompAttr(self):
        return self.__Comp_Attr

#**********************************************************************
    #Handling Bus Current
    def setBusCurrent(self,BusCurrent):
        self.__Bus_Current = BusCurrent

    def getBusCurrent(self):
        return self.__Bus_Current

#**********************************************************************
    #Handling Product Serial Number
    def setProductSN(self,P_SN):
        self.__Product_Serial = P_SN

    def getProductSN(self):
        return self.__Product_Serial

#**********************************************************************
   #Handling Component Type Number
    def setCompTypeNo(self,CompTypeNo):
        self.__CompTypeNo  = CompTypeNo

    def getCompTypeNo(self):
        return self.__CompTypeNo

#**********************************************************************
   #Handling Product Picture
    def setProductPic(self,P_Pic):
        self.__ProductPic = P_Pic

    def getProductPic(self):
        return self.__ProductPic

#**********************************************************************
    #BCU Type
    def setBCUType(self,BCUType):
        self.__BCU_Type = BCUType

    def getBCUType(self):
        return self.__BCU_Type

#**********************************************************************
    #Product Handling
    def setProductHandling(self,P_Handling):
        self.__Product_Handling = P_Handling

    def getProductHandling(self):
        return self.__Product_Handling

#**********************************************************************
    #Product DLL
    def setProductDLL(self,P_DLL):
        self.__ProductDLL = P_DLL

    def getProductDLL(self):
        return self.__ProductDLL

#**********************************************************************
    #Original Manufacturer ID
    def setOrigManID(self,O_ManID):
        self.__OrigManID = O_ManID

    def getOrigManID(self):
        return self.__OrigManID
 #**********************************************************************
