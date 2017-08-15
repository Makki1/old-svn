#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_Project.py
#Version: V0.1 , 28.01.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import os
from xml.dom import minidom
from xml.dom.minidom import *
from FB_PROJECT import FB_ArchitecturalDataModel
#from xml.dom.minidom import getDOMImplementation

##general class for handling project data which are based on XML
class FB_Project:


    __LogObj = None
    __archmodel = None        #object: architectural model
    __instamodell = None      #object: installation model
   # __parent = None           #object: Application

    ##Constructor for new Project
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param projectname: Path and name of project
    def __init__(self, LogObj, projectname, projectpath):
        self.__LogObj = LogObj
        ArchDocument = None

        #create new directory of projectname -> for opening projects
        if(projectpath != None):
            ArchDocument = minidom.parse(projectpath + "\\structure.xml")
            self.__archmodel = FB_ArchitecturalDataModel.FB_ArchitecturalDataModel(self.__LogObj,ArchDocument,projectname,False)
        #new project
        else:
            #create empty structure file
            impl = getDOMImplementation()
            ArchDocument = impl.createDocument(None, "architectural-data", None)
            self.__archmodel = FB_ArchitecturalDataModel.FB_ArchitecturalDataModel(self.__LogObj,ArchDocument,projectname,True)

        #XMLLoader load2=new XMLLoader(new File(projectpath.getPath()+"/installation.xml"));
        #instamodell=new InstallationModel(load2.getDocument());
  
        
#*****************************************************************************
    def getArchModel(self):
        return self.__archmodel
#*****************************************************************************
    def setProjectPath(self,projectPath):
        self.__archmodel.setProjectPath(projectPath)

    def getProjectPath(self):
        return self.__archmodel.getProjectPath()
#*****************************************************************************
    def setProjectName(self, projectName):
        self.__archmodel.setProjectName(projectName)

    def getProjectName(self):
        return self.__archmodel.getProjectName()
#*****************************************************************************
    def setComment(self, comment):
        self.__archmodel.setComment(comment)

    def getComment(self):
        return self.__archmodel.getComment()
#*****************************************************************************
    def getProjectDirectoryName(self):
        return self.__archmodel.getProjectDirectoryName()
#*****************************************************************************
    def setPrefferedBusSystem(self, prefferedBusSystem):
        self.__archmodel.setPrefferedBusSystem(prefferedBusSystem)

    def getPrefferedBusSystem(self):
        return self.__archmodel.getPrefferedBusSystem()
#*****************************************************************************

    def SaveProject(self,):
        self.__archmodel.SaveArchmodel()
