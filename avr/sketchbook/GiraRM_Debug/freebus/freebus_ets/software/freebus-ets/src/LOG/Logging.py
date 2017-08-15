#-*- coding: iso-8859-1 -*-
#!/usr/bin/
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source Datei: Logging.py
#Version: V0.1 vom 10.11.2007
#Autor: Jerome Leisner
#email: j.leisner@ing-automation.de
#
#===============================================================================

#TODO: Pdad/Ordnerprüfungen anlegen

from time import *


class Logging:

    _OutFileObj = None
    _ClassObj = None
    _Options = 0
    _FileName = ""

    __LogType = ["Meldung","Fehler","Exception"]
    #Konstruktor der Logging-Klasse
    def __init__(self,ClassObj,LogFileName,Options):
        #ClassObj: Absender (KLassenname)
        #LogFileName: Pafd und Name der Zieldatei
        #Options: z.Bsp. 8-Bits zum Schreiben verschiedener Optionen (Datum,Zeit...)
        self._FileName = LogFileName

        self._ClassObj = ClassObj
        self._Options = Options



    def NewLog(self,LogText,LogType):
        #LogType: Typ des Eintrags 0... (z.Bsp: Meldung,Fehler, Exceptions...)
        #LogType 1 : Meldung
        #LogType 2 : Fehler
        #LogType 3 : Exception
        self._OutFileObj = open(self._FileName,"w")

        TimeStr = ctime()

        TimeStr =  TimeStr.split()
        TimeStr = TimeStr[2]+ ". " + TimeStr[1]+" "+TimeStr[4]+" " +TimeStr[3]
        LogStr = self._ClassObj + "; " + TimeStr + "; " + self.__LogType[LogType] + "; " + LogText + "\n"
        self._OutFileObj.write(LogStr)
        self._OutFileObj.close
