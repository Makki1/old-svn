#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_XMLHandler.py
#Version: V0.1 , 25.12.2007
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#
#===============================================================================

from xml.dom.minidom import *
import codecs
import xml.sax
import sys, string
from XML.FB_XML_HANDLER import FB_ProductXMLHandler
from XML.FB_XML_HANDLER import FB_AppXMLHandler
from XML.FB_XML_HANDLER import FB_CommXMLHandler
from XML.FB_XML_HANDLER import FB_Prod2ProgrXMLHandler
from XML.FB_XML_HANDLER import FB_MaskXMLHandler

##Class for Extracting Products from FB-XML Productfiles
class FB_XMLHandler(xml.sax.ContentHandler):

    __LogObj = None
    __products = []
    __prod = None
    __prod_XMLHandler = None         #private object of FB_ProductXMLHandler
    __app_XMLHandler = None          #private object of FB_AppXMLHandler
    __comm_XMLHandler = None         #private object of FB_CommXMLHandler
    __prod2progr_XMLHandler = None   #privtae object of FB_Prod2ProgrXMLHandler
    __mask_XMLHandler = None         #private object of FB_Mask
    __locator = None

    #Constructor for FB_XMLHandler.
    def __init__(self, LogObj):
        self.__LogObj = LogObj
        #Product-Handling
        self.__prod_XMLHandler = FB_ProductXMLHandler.FB_ProductXMLHandler(self.__LogObj)
        #Application-Handling
        self.__app_XMLHandler = FB_AppXMLHandler.FB_AppXMLHandler(self.__LogObj)
        #Communication-objects Handling
        self.__comm_XMLHandler = FB_CommXMLHandler.FB_CommXMLHandler(self.__LogObj)
        #Product to Program objects Handling
        self.__prod2progr_XMLHandler = FB_Prod2ProgrXMLHandler.FB_Prod2ProgrXMLHandler(self.__LogObj)
        #Mask object Handling
        self.__mask_XMLHandler = FB_MaskXMLHandler.FB_MaskXMLHandler(self.__LogObj)

    #return List of Instances of type FB_Products
    def getProductList(self):
        return self.__prod_XMLHandler.getProductList()

    #return List of Instances of type FB_Applications
    def getAppList(self):
        return self.__app_XMLHandler.getAppList()

    #return List of Instances of type FB_Manufacturer
    def getManufacturerList(self):
        return self.__app_XMLHandler.getManufacturerList()

    #return List of Instances of type FB_CommunicationObj
    def getCommObjList(self):
        return self.__comm_XMLHandler.getCommObjList()

    #return List of INstances of type FB_Prod2Progr
    def getProd2ProgrList(self):
        return self.__prod2progr_XMLHandler.getProduct2ProgramList()

    #return List of Instances of type FB_Mask
    def getMaskList(self):
        return self.__mask_XMLHandler.getMaskList()

    #return Locator-Data (position of parser)
    def getLocator(self):
        #print self.__locator.getLineNumber
        return self.__locator



    def startDocument(self):
        try:
            #print "Start"
            pass

        except SAXException:
            print "SAXError"

    def endDocument(self):
        try:
            #print "Ende"
            pass

        except SAXException:
            print "SAXError"



    def startElement(self, eName, attrs):
        try:
           # if(self.__locator != None):
           #     print self.__locator.getLineNumber()
           #     print self.__locator.getColumnNumber()

            self.__prod_XMLHandler.startElement(eName, attrs)
            self.__app_XMLHandler.startElement(eName, attrs)
            self.__comm_XMLHandler.startElement(eName, attrs)
            self.__prod2progr_XMLHandler.startElement(eName, attrs)
            self.__mask_XMLHandler.startElement(eName, attrs)

        except SAXException:
            print "Error again"

    def endElement(self,eName):
        try:
            self. __prod_XMLHandler.endElement(eName)
            self.__app_XMLHandler.endElement(eName)
            self.__comm_XMLHandler.endElement(eName)
            self.__prod2progr_XMLHandler.endElement(eName)
            self.__mask_XMLHandler.endElement(eName)

        except SAXException:
            print "Error again"


    def characters(self ,char):
        try:
            self.__prod_XMLHandler.characters(char)
            self.__app_XMLHandler.characters(char)
            self.__comm_XMLHandler.characters(char)
            self.__prod2progr_XMLHandler.characters(char)
            self.__mask_XMLHandler.characters(char)

        except SAXException:
             print "Error again"

    def setDocumentLocator(self, locator):
        #print "set"
        self.__locator = locator


#  private void print(String label, SAXParseException e)
#  {
#     System.err.println("** Parser " + label + ": " +
#                        e.getMessage());
#     System.err.println("   Line:      " + e.getLineNumber());
#     System.err.println("   Column:    " + e.getColumnNumber());
#     System.err.println("   URI:       " + e.getSystemId());
#     System.err.println("   PUBLIC id: " + e.getPublicId());
#  }
