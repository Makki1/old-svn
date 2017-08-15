#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_EIBPhAddress.py
#Version: V0.1 , 28.08.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================





###EIB physical addresses
class FB_EIBAddress(FB_EIBAddress):




    ##Constructor
    def __init__(self):
        self.addr_h = 0x0
        self.addr_l = 0x0

    ##Sets EIB physical address with a string
    #@param s string with an EIB group address
    def __init__(self,s):
        self.setAddress(s)

    ##Set EIB physical addres form three values
    #@param zone device zone number
    #@param line device line number
    #@param device device number
    def __init__(self,zone,line,device):
        self.setAddress(zone, line, device)

    ##Set EIB physical address from a string (e.g. 1.1.1)
    def setAddress(self,s):
        try:
            pass
        except:
            #toDo: Wrong address format!
            pass

    ##Set EIB physical addres form three values
    def setAddress(self,zone, line, device):
        try:
            self.addr_h = (zone << 4) | line
            self.addr_l = device
        except:
            #toDo: Wrong address format!
            pass

    ##Get a string representation of the address
    def ToString(self):
        s = (self.addr_h >> 4)+"."+(self.addr_h & 0x0F)+"."+self.addr_l
        return s