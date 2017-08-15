#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_Prod2Progr.py
#Version: V0.1 , 03.02.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

##Class for Handling of Product to Program ID's
class FB_Prod2Prog:


    __Prod2Prog_ID = ""           #PROD2PROG_ID
    __Product_ID = ""             #PRODUCT_ID
    __Program_ID = ""             #PROGRAM_ID
    __Prod2Prog_Status_Code = ""  #PROD2PROG_STATUS_CODE
    __Reg_Number = ""             #REGISTRATION_NUMBER
    __Reg_Year = ""               #REGISTRATION_YEAR
    __Orig_Reg_Number = ""        #ORIGINAL_REGISTRATION_NUMBER
    __Orig_Reg_Year = ""          #ORIGINAL_REGISTRATION_YEAR
    __Reg_TS = ""                 #REGISTRATION_TS



    def __init__(self):
        pass

    def setProd2Prog(self,Index, Value):
        if(Index == 1):
            self.__Prod2Prog_ID = Value
        elif(Index == 2):
            self.__Product_ID = Value
        elif(Index == 3):
            self.__Program_ID = Value
        elif(Index == 4):
            self.__Prod2Prog_Status_Code = Value
        elif(Index == 5):
            self.__Reg_Number = Value
        elif(Index == 6):
            self.__Reg_Year = Value
        elif(Index == 7):
            self.__Orig_Reg_Number = Value
        elif(Index == 8):
            self.__Orig_Reg_Year = Value
        elif(Index == 9):
            self.__Reg_TS = Value

    def getProd2Prog(self,Index):
        if(Index == 1):
            return self.__Prod2Prog_ID
        elif(Index == 2):
            return self.__Product_ID
        elif(Index == 3):
            return self.__Program_ID
        elif(Index == 4):
            return self.__Prod2Prog_Status_Code
        elif(Index == 5):
            return self.__Reg_Number
        elif(Index == 6):
            return self.__Reg_Year
        elif(Index == 7):
            return self.__Orig_Reg_Number
        elif(Index == 8):
            return self.__Orig_Reg_Year
        elif(Index == 9):
            return self.__Reg_TS

#**********************************************************************
    #Handling PROD2PROG_ID
    def setProd2ProgID(self,PP_ID):
        self.__Prod2Prog_ID = PP_ID

    def getProd2ProgID(self):
        return self.__Prod2Prog_ID

#**********************************************************************
    #Handling Product ID
    def setProductID(self,P_ID):
        self.__Product_ID = P_ID

    def getProductID(self):
        return self.__Product_ID
#**********************************************************************
    #Handling PROGRAM_ID
    def setProgramID(self,Pr_ID):
        self.__Program_ID = Pr_ID

    def getProgramID(self):
        return self.__Program_ID

#**********************************************************************
    #Handling PROD2PROG_STATUS_CODE
    def setProd2ProgStatusCode(self,S_Code):
        self.__Prod2Prog_Status_Code = S_Code

    def getProd2ProgStatusCode(self):
        return self.__Prod2Prog_Status_Code

#**********************************************************************
    #Handling REGISTRATION_NUMBER
    def setRegNumber(self,R_Number):
        self.__Reg_Number = R_Number

    def getRegNumber(self):
        return self.__Reg_Number

#**********************************************************************

    #Handling REGISTRATION_YEAR
    def setRegYear(self,R_Year):
        self.__Reg_Year = R_Year

    def getRegYear(self):
        return self.__Reg_Year

#**********************************************************************

    #Handling ORIGINAL_REGISTRATION_NUMBER
    def setOrigRegNumber(self,Orig_RegNumber):
        self.__Orig_Reg_Number = Orig_RegNumber

    def getOrigRegNumber(self):
        return self.__Orig_Reg_Number

#**********************************************************************
    #Handling ORIGINAL_REGISTRATION_YEAR
    def setOrigRegYear(self,Orig_RegYear):
        self.__Orig_Reg_Year = Orig_RegYear

    def getOrigRegYear(self):
        return self.__Orig_Reg_Year

#**********************************************************************
    #Handling REGISTRATION_TS
    def setRegTS(self,RegTS):
        self.__Reg_TS = RegTS

    def getRegTS(self):
        return self.__Reg_TS

#**********************************************************************
