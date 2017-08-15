#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source Datei: FB_ErrorXMLHandler.py
#Version: V0.1 vom 15.11.2007
#Autor: Jerome Leisner
#email: j.leisner@ing-automation.de
#
#Hilfsklasse zur Fehlerbearbeitung mit dem SAX-Parser (XML)
#===============================================================================


import codecs
import xml.sax

class FB_ErrorXMLHandler():


    def __init__(self):
        pass



    def error(self, exception):
        print "Error " + excpetion


    def fatalError(self, exception):
        print "Fatal-Error " + excpetion

    def warning(self, exception):
        print "Warning " + excpetion





