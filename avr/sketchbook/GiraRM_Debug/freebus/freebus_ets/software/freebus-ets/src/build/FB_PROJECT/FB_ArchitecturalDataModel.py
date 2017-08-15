#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_ArchitecturalDataModel.py
#Version: V0.1 , 27.01.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================


from xml.dom import minidom
from xml.dom.minidom import *
from XML.FB_XMLDataModel import FB_XMLDataModel #import class as base class
import os


##general class for handling project data which are based on XML
class FB_ArchitecturalDataModel(FB_XMLDataModel):


    __LogObj = None
    __DOMObj = None
    __ROOT_ID = ""
    __PROJECT_PREFIX=""
    __BUILDING_PREFIX=""
    __FLOOR_PREFIX=""
    __ROOM_PREFIX=""
    __JUNCTION_BOX_PREFIX=""
    __archDocument = None
    __PATH = ""         #Project Path

    ##Constructor
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param projectname: Path and name of project
    def __init__(self, LogObj, ArchDocument,projectname, new):
        FB_XMLDataModel.__init__(self,LogObj,ArchDocument,projectname)

        self.__LogObj = LogObj
        self.__archDocument = ArchDocument
        self.__ROOT_ID = "project-1"
        self.__PROJECT_PREFIX = "project"
        self.__BUILDING_PREFIX = "building"
        self.__FLOOR_PREFIX = "floor"
        self.__ROOM_PREFIX = "room"
        self.__JUNCTION_BOX_PREFIX = "junctionbox"

        #archNode = Document.appendChild(Document.createElement("architectural-data"))
        #find mainnode "architectural-data"
        if(new == True):
            archNode = self.__archDocument.documentElement
            pNode = archNode.appendChild(self.__archDocument.createElement("project"))

            pNode.setAttribute("id", self.__ROOT_ID)
            pNode.appendChild(self.__archDocument.createElement("name")).appendChild(self.__archDocument.createTextNode(projectname))
            pNode.appendChild(self.__archDocument.createElement("comment"))
            DirTextNode = self.__archDocument.createTextNode(self.makeProjectDirectoryName())
            pNode.appendChild(self.__archDocument.createElement("directoryname")).appendChild(DirTextNode)
            pNode.appendChild(self.__archDocument.createElement("preffered-bus-system"))

            OutFileObj = open("structure.xml","w")
            String = self.__archDocument.toxml(encoding = "ISO-8859-1")
            OutFileObj.write(String)
            OutFileObj.close()

    ##get the prefix-string with a given index
    #@param Index: 0 for project, 1 for building ...
    def getPrefix(self,Index):

        if(Index == 1):
            return self.__PROJECT_PREFIX
        elif(Index == 2):
            return self.__BUILDING_PREFIX
        elif(Index == 3):
            return self.__FLOOR_PREFIX
        elif(Index == 4):
            return self.__ROOM_PREFIX
        elif(Index == 5):
            return self.__JUNCTION_BOX_PREFIX

    ##Returns the data model root node ID.
    def getRootID(self):
        return self.__ROOT_ID;


    def getChildIDs(self, parentID):
        if(parentID.find(self.__PROJECT_PREFIX) > -1):

            return self.getIDList(self.getDataRootNode(parentID), self.__BUILDING_PREFIX)

        elif(parentID.find(self.__BUILDING_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.__FLOOR_PREFIX)

        elif(parentID.find(self.__FLOOR_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.__ROOM_PREFIX)

        elif(parentID.find(self.__BUILDING_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.__FLOOR_PREFIX)

        elif(parentID.find(self.__FLOOR_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.__ROOM_PREFIX)

        elif(parentID.find(self.__ROOM_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.__JUNCTION_BOX_PREFIX)
        else:
            return None
#****************************************************************************
    def getParentID(self, childID):
        #child ID starts not with Project-Prefix
        if(childID.find(self.__PROJECT_PREFIX) == -1):
            ParentElement = self.getDataRootNode(childID).parentNode()
            return ParentElement.getAttribute("id")
        else:
            return ""

#****************************************************************************
    def addChild(self, parentID):
        parent = self.getDataRootNode(parentID)
        if(parentID.find(self.__PROJECT_PREFIX) > -1):
            Element = self.createCild(self.__BUILDING_PREFIX)
            parent.appendChild(Element)
            return self.getChildID(Element)

        elif(parentID.find(self.__BUILDING_PREFIX) > -1):
            Element = self.createCild(self.__FLOOR_PREFIX)
            parent.appendChild(Element)
            return self.getChildID(Element)

        if(parentID.find(self.__FLOOR_PREFIX) > -1):
            Element = self.createCild(self.__ROOM_PREFIX)
            parent.appendChild(Element)
            return self.getChildID(Element)

        if(parentID.find(self.__ROOM_PREFIX) > -1):
            Element = self.createCild(self.__JUNCTION_BOX_PREFIX)
            parent.appendChild(Element)
            return self.getChildID(Element)
        else:
            return None

#****************************************************************************
    def addEndDevice(self, roomId, enddevId, Point , devType):
        pass
        #Node = self.getDataRootNode(roomId)

        #Document doc=n.getOwnerDocument();

        #Element = n.getOwnerDocument().createElement("enddevice");

        #Element.setAttribute("id", enddevId)

        #self.writeDOMNodeValue(ed, new StringTokenizer("type", "/"), devType+"");

        #n.appendChild(ed);

#****************************************************************************
    def getEndDeviceID(self, endDeviceNode):
        return endDeviceNode.getAttribute("id")
#****************************************************************************
     ##Add bus device link to installation location (Room or Junction Box).
     #@param installationLocationId room or junction box id.
     #@param devId device id in installation.xml
    def addBusDevice(self, installationLocationId, devId):
        Node = self.getDataRootNode(installationLocationId)

       # Document doc=n.getOwnerDocument();

       # Element ed=n.getOwnerDocument().createElement("busdevice");

      #  ed.appendChild(doc.createTextNode(devId));

       # n.appendChild(ed);
#****************************************************************************
     ##Returns ids of all installed bus devices in the given installation location.
     #@param installationLocationID id of the installation location to search for devices
     #@return ids of matching bus devices.
    def getBusDeviceIDs(self, installationLocationID):
        Element = self.getDataRootNode(installationLocationID)
        NodeList = Element.getElementsByTagName("busdevice")

        for cnt in range(len(NodeList)):
            Node = NodeList.item(cnt)
            #v.addElement(node.getFirstChild().getNodeValue());

        return v
#****************************************************************************
    def getInstallationLocationName(self, busdeviceID):
        name = ""
        Node = None
        NodeList = None #self.getDocumnet().getElementsByTagName("busdevice");

        for cnt in range(len(NodeList)):
            con = None
            try:
                con= NodeList.item(cnt).firstChild().nodeValue

                if(con == busdeviceID):
                    n = NodeList.item(cnt).parentNode()
                    break
            except:
           # catch(NullPointerException npe)
                   pass

        if(n != None):
            name = self.readDOMNodeValue(n, "name")

        return name
#****************************************************************************
    def setProjectPath(self,projectPath):
        self.__PATH = projectPath

    def getProjectPath(self):
        return self.__PATH
#****************************************************************************
    def getProjectName(self):
        return self.getName(self.__ROOT_ID)

    def setProjectName(self, name):
        self.setName(self.__ROOT_ID, name)
#****************************************************************************
    def getComment(self):
        Node = self.getDataRootNode(self.__ROOT_ID)
        return self.readDOMNodeValue(Node, "comment")

    def setComment(self, comment):
        Node = self.getDataRootNode(self.__ROOT_ID)
        self.writeDOMNodeValue(Node, "comment", comment)
#****************************************************************************
    def getPrefferedBusSystem(self):
        Node = self.getDataRootNode(self.__ROOT_ID)
        return self.readDOMNodeValue(Node, "preffered-bus-system")

    def setPrefferedBusSystem(self, pbs):
        Node = self.getDataRootNode(self.__ROOT_ID)
        self.writeDOMNodeValue(Node, "preffered-bus-system", pbs)
#****************************************************************************
    def getProjectDirectoryName(self):
        Node = self.getDataRootNode(self.__ROOT_ID);
        return self.readDOMNodeValue(Node, "directoryname")
#****************************************************************************
    def makeProjectDirectoryName(self):
        name = self.getProjectName()
        name = name.replace(" ", "_")
        #print name
        return name
#****************************************************************************
    def SaveArchmodel(self):
        os.chdir(self.getProjectPath())
        OutFileObj = open("structure.xml","w")
        String = self.__archDocument.toxml(encoding = "ISO-8859-1")
        OutFileObj.write(String)
        OutFileObj.close()
