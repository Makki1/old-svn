#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_EIBProgrammer.py
#Version: V0.1 , 29.08.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================


class FB_EIBProgrammer(object):

    CONNECTION_NOT_POSSIBLE = 1        #some static constants
    CONNECTION_WAS_CLOSED   = 2
    WRONG_MASK_VERSION      = 3
    WRONG_MANUFACTURER_ID   = 4
    MEMORY_VERIFY_PROBLEM   = 5
    CANCEL                  = 6

    __isCancled = False
    __progress = 0
    __errorcode = 0
    __con = None

    _goOn = True    # set false if you want to cancel the Programming

    def startProgramming(self):
        pass

    ##sets the current eib connection
    #@param con: connection object
    def setEIBConnection(self,con):
        self.con = con

    def cancelProgramming(self):
        self._goOn = False

    def abortProgramming(self):
        self.cancelProgramming()
        self.setErrorCode(CANCEL)


    def getEIBConnection(self):
        return self.con


    def isCancled(self):
        return self.__isCancled

    def setIsCancled(self,isCancled):
        self.__isCancled = isCancled

    def getProgress(self):
        return self.__progress

    def setProgress(self,progress):
        self.__progress = progress


    def setErrorCode(self,errorcode):
        self.__errorcode = errorcode

    def getErrorCode(self):
        return self.__errorcode