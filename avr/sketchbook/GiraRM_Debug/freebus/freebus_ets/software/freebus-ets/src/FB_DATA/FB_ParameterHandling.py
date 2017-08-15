#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_ParameterHandling.py
#Version: V0.1 , 19.07.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

##Class for Handling of Product Parameter
class FB_ParameterHandling:


    __SelProductData = None
    __SelApplicationData = None


    ##Constructor of class ParameterHandling
    #@param SelProductData: complete List of current selected Product (Database-Data)
    #@param SelApplicationData: complete List of current selected Application (Database-Data)
    def __init__(self, SelProductData, SelApplicationData):
        self.__SelProductData = SelProductData
        self.__SelApplicationData = SelApplicationData


    #def GetDefault