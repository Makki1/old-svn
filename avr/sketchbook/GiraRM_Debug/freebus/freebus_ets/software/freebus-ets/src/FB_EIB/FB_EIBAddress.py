#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_EIBAddress.py
#Version: V0.1 , 28.08.2009
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================


class FB_EIBAddress(object):

    _addr_h = 0
    _addr_l = 0


    ##Constructor
    def __init__(self):
        pass

    ###Parse EIB Address from a String
    ##@param s: Addressstring to parse
    def setAddress(self,s):
        pass

    ###Get EIB Address high byte
    def getHigh(self):
        return self._addr_h

    ###Get EIB Address low byte
    def getLow(self):
        return self._addr_l

    ###Set EIB Address high byte
    ##@param addr_h:high byte address value
    def setHigh(self,addr_h):
        self._addr_h = addr_h

    ###Set EIB Address low byte
    ##@param addr_l:low byte address value
    def setLow(self,addr_l):
        self._addr_l = addr_l

    ###Compare EIBAddress objects
    ##@param ea EIBAddress object to compare with actual object
    ##@return true if equals.
    def equals(self,ea):
        if ea.getHigh() is self.getHigh() and  ea.getLow() is self.getLow():
            return true
        else:
            return false

    ###Returns true if this address is greater than the given address.
    ##@param ea the address to compare
    ##@return true if this address is greater than the given address
    def isGreaterThan(self, ea):
        l1 = ea.getHigh()<<8 | (ea.getLow() & 0xFF)
        l2 = self.getHigh()<<8 | (self.getLow() & 0xFF)

        return l2 > l1

