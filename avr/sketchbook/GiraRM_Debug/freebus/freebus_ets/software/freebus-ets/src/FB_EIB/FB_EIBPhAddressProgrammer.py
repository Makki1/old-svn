#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_EIBPhAddressProgrammer.py
#Version: V0.1 , 29.08.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================
from threading import *

class FB_EIBPhAddressProgrammer(FB_EIBProgrammer):


    MULTIBUTTON = 1
    ADDRESSINUSE = 2
    NOBUTTONPRESSED = 3

    __t = None            #Thread
    __con = None          #EIBConnection
    __addr = None         #EIBPhaddress


    ##Constructor
    def __init__(self,con,addr):
        self.__con = con
        self.__addr = addr

    def startProgramming(self):
        InternalThread(self)


class InternalThread(Thread):

    __p = None        #EIBProgrammer

    def __init__(self,p):
        Thread.__init__(self)
        self.__p = p
        self.start()

    ##run the programming thread
    def run(self):

        #prog =
        #EIBSystemProgrammer prog=new EIBSystemProgrammer(con, con.getIndividualAddress(), addr);

        setProgress(10)


