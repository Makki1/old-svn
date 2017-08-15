#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_AdressLogic.py
#Version: V0.1 , 04.06.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
from xml.dom import minidom
from xml.dom.minidom import *

#from xml.dom.minidom import getDOMImplementation

##general class for handling the entire adresslogic within the project
class FB_AdressLogic:

    __LogObj = None
    __ProjectObj = None
    __ArchModel = None    #obj of the architectural structure of the project
    __MaxLineAdress = 0
    __MaxAreaAdress = 0
    __MaxDevice = 0
    __MaxMainGroup = 0
    __MaxMiddleGroup = 0
    __MaxGroupAdress = 0
    __GenMinAdress = 0
    __DeviceMinAdress = 1


    ##Constructor for new Project
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param projectname: Path and name of project
    def __init__(self, LogObj, ProjectObj):
        self.__LogObj = LogObj
        self.__ProjectObj = ProjectObj
        self.__ArchModel = self.__ProjectObj.getArchModel()
        self.__MaxLineAdress = 16 #0..15
        self.__MaxAreaAdress = 16 #0..15
        self.__MaxMainGroup = 16  #0..15
        self.__MaxMiddleGroup = 8 #0..7
        self.__MaxGroupAdress = 256 #0..255
        self.__MaxDevice = 255 #[1..255]
        self.__GenMinAdress = 0
        self.__DeviceMinAdress = 1

     #checks if the given Adress is still posible
    def CheckAdress(self,Adress, Prefix,ParentID):
        RValue = True

        if(Prefix == self.__ArchModel.TOPOLOGY_AREA):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_AREA)
        elif(Prefix == self.__ArchModel.TOPOLOGY_LINE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_LINE)
        elif(Prefix == self.__ArchModel.TOPOLOGY_DEVICE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_DEVICE)

        elif(Prefix == self.__ArchModel.GROUPADRESS_MAIN):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS_MAIN)
        elif(Prefix == self.__ArchModel.GROUPADRESS_MIDDLE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS_MIDDLE)
        elif(Prefix == self.__ArchModel.GROUPADRESS):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS)


        for obj in range(len(List)):
            curAdress = self.__ArchModel.getAdress(List[obj])
            if(str(curAdress) == str(Adress)):
                RValue = False
                break

        return RValue

    #gets the next possible Area-Adress
    #@param Prefix: Prefix to identify the ardesstype (area, line, ...)
    #@param Direction: True = positiv, False = negativ
    def GetNextPossibleAdress(self, Prefix,StartAdress,Direction,ParentID):
        #init
        RValue = StartAdress
        NextPossibleAdress = StartAdress
        OriginAdress = StartAdress
        MaxAdress = 0
        MinAdress = 0
        AdressListe = []
        Valid = False
        NoValue = False    #Value not in List

        if(Prefix == self.__ArchModel.TOPOLOGY_AREA):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_AREA)
            MaxAdress = self.GetMaxAreaAdress() - 1
            MinAdress = self.__GenMinAdress
        elif(Prefix == self.__ArchModel.TOPOLOGY_LINE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_LINE)
            MaxAdress = self.GetMaxLineAdress() - 1
            MinAdress = self.__GenMinAdress
        elif(Prefix == self.__ArchModel.TOPOLOGY_DEVICE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_DEVICE)
            MaxAdress = self.GetMaxDeviceAdress()
            MinAdress = self.__DeviceMinAdress

        elif(Prefix == self.__ArchModel.GROUPADRESS_MAIN):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS_MAIN)
            MaxAdress = self.GetMaxMainGroupAdress() - 1
            MinAdress = self.__GenMinAdress
        elif(Prefix == self.__ArchModel.GROUPADRESS_MIDDLE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS_MIDDLE)
            MaxAdress = self.GetMaxMiddleGroupAdress() -1
            MinAdress = self.__GenMinAdress
        elif(Prefix == self.__ArchModel.GROUPADRESS):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS)
            MaxAdress = self.GetMaxGroupAdress() - 1
            MinAdress = self.__GenMinAdress

        for obj in range(len(List)):
            curAdress = self.__ArchModel.getAdress(List[obj])
            AdressListe.append(curAdress)

        AdressListe.sort()

        while Valid == False:
            NoValue = False
            for Adress in range(len(AdressListe)):
                if(str(NextPossibleAdress) == str(AdressListe[Adress])):
                    NoValue = True

            #if adress is already in list -> inkrement adress and test it again
            if(NoValue == True):
                if(Direction == True):
                    NextPossibleAdress = NextPossibleAdress + 1
                    if(StartAdress > MinAdress):
                        OriginAdress = StartAdress - 1
                    else:
                        OriginAdress = MinAdress

                else:
                    NextPossibleAdress = NextPossibleAdress - 1
                    if(StartAdress < MaxAdress):
                        OriginAdress = StartAdress + 1
                    else:
                        OriginAdress = MaxAdress


                #check if NextPossibleAdress is still in bounds
                #upper bound
                if(NextPossibleAdress <= MaxAdress):
                    RValue = NextPossibleAdress
                else:
                    RValue = OriginAdress

                #lower bound
                if(NextPossibleAdress < MinAdress):
                    RValue = OriginAdress

            else:
                Valid = True


        return RValue

    #gets a bit if a new Area/Line/.... is possible
    def MoreAdressPossible(self,Prefix,ParentID):
        RValue = False
        List = []
        MaxAdress = 0

        if(Prefix == self.__ArchModel.TOPOLOGY_AREA):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_AREA)
            MaxAdress = self.GetMaxAreaAdress()
        elif(Prefix == self.__ArchModel.TOPOLOGY_LINE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_LINE)
            MaxAdress = self.GetMaxLineAdress()
        elif(Prefix == self.__ArchModel.TOPOLOGY_DEVICE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.TOPOLOGY_DEVICE)
            MaxAdress = self.GetMaxDeviceAdress()

        elif(Prefix == self.__ArchModel.GROUPADRESS_MAIN):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS_MAIN)
            MaxAdress = self.GetMaxMainGroupAdress()
        elif(Prefix == self.__ArchModel.GROUPADRESS_MIDDLE):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS_MIDDLE)
            MaxAdress = self.GetMaxMiddleGroupAdress()
        elif(Prefix == self.__ArchModel.GROUPADRESS):
            List = self.__ArchModel.getIDList(self.__ArchModel.getDataRootNode(ParentID),self.__ArchModel.GROUPADRESS)
            MaxAdress = self.GetMaxGroupAdress()


        if(len(List) < MaxAdress):
            RValue = True
        else:
            RValue = False

        return RValue

    def GetMaxAreaAdress(self):
        return self.__MaxAreaAdress

    def GetMaxLineAdress(self):
        return self.__MaxLineAdress

    def GetMaxDeviceAdress(self):
        return self.__MaxDevice

    def GetMaxMainGroupAdress(self):
        return self.__MaxMainGroup

    def GetMaxMiddleGroupAdress(self):
        return self.__MaxMiddleGroup

    def GetMaxGroupAdress(self):
        return self.__MaxGroupAdress

