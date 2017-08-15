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
    BUILDING_PREFIX=""
    FLOOR_PREFIX=""
    ROOM_PREFIX=""
    JUNCTION_BOX_PREFIX=""
    TOPOLOGY_ROOT=""
    TOPOLOGY_AREA=""
    TOPOLOGY_LINE=""
    TOPOLOGY_DEVICE=""
    GROUPADRESS_ROOT=""
    GROUPADRESS_MAIN=""
    GROUPADRESS_MIDDLE=""
    GROUPADRESS=""
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
        self.TOPOLOGY_ROOT_ID = "topology-1"
        self.GROUPADRESS_ROOT_ID = "groupadress-1"
        self.__PROJECT_PREFIX = "project"
        self.BUILDING_PREFIX = "building"
        self.FLOOR_PREFIX = "floor"
        self.ROOM_PREFIX = "room"
        self.JUNCTION_BOX_PREFIX = "junctionbox"

        self.TOPOLOGY_AREA = "AreaTopology"
        self.TOPOLOGY_LINE = "LineTopology"
        self.TOPOLOGY_DEVICE = "Device"

        self.GROUPADRESS_MAIN = "MainGroup"
        self.GROUPADRESS_MIDDLE = "MiddleGroup"
        self.GROUPADRESS = "GroupAdress"

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
            newNode = pNode.appendChild(self.__archDocument.createElement("topology"))
            newNode.setAttribute("id", self.TOPOLOGY_ROOT_ID)
            newNode = pNode.appendChild(self.__archDocument.createElement("groupadress"))
            newNode.setAttribute("id", self.GROUPADRESS_ROOT_ID)



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
            return self.BUILDING_PREFIX
        elif(Index == 3):
            return self.FLOOR_PREFIX
        elif(Index == 4):
            return self.ROOM_PREFIX
        elif(Index == 5):
            return self.JUNCTION_BOX_PREFIX
        elif(Index == 20):
            return self.TOPOLOGY_AREA
        elif(Index == 21):
            return self.TOPOLOGY_LINE
        elif(Index == 30):
            return self.GROUPADRESS_MAIN
        elif(Index == 31):
            return self.GROUPADRESS_MIDDLE
        elif(Index == 32):
            return self.GROUPADRESS

    #returns the title of the ne structure element dialog
    #return structur:
    #Title of the dialog, AdressField visible or not, Adresslabelbezeichnung ,maximum adress
    def getTypeStructureElement(self,Prefix):
        if(Prefix == self.BUILDING_PREFIX):
            return unicode("neues Gebäude anlegen...","ISO-8859-1"), False, "", 0
        if(Prefix == self.FLOOR_PREFIX):
            return unicode("neue Etage anlegen...","ISO-8859-1"), False, "", 0
        if(Prefix == self.ROOM_PREFIX):
            return unicode("neuen Raum anlegen...","ISO-8859-1"), False, "", 0
        if(Prefix == self.JUNCTION_BOX_PREFIX):
            return unicode("neuen Verteiler anlegen...","ISO-8859-1"), False, "", 0
        if(Prefix == self.TOPOLOGY_AREA):
            return unicode("neuen Bereich anlegen...","ISO-8859-1"),True, "Breichsadresse [0..15] :" , 15
        if(Prefix == self.TOPOLOGY_LINE):
            return unicode("neue Linie anlegen...","ISO-8859-1"),True, "Linieadresse [0..15]: ", 15
        if(Prefix == self.GROUPADRESS_MAIN):
            return unicode("neue Hauptgruppe anlegen...","ISO-8859-1"),True, "Hauptgruppe [0..15]: ", 15
        if(Prefix == self.GROUPADRESS_MIDDLE):
            return unicode("neue Mittelgruppe anlegen...","ISO-8859-1"),True, "Mittelgruppe [0..7]: ", 7
        if(Prefix == self.GROUPADRESS):
            return unicode("neue Gruppenadresse anlegen...","ISO-8859-1"),True, "Gruppenadresse [0..255]: ", 255


    ##Returns the data model root node ID.
    def getRootID(self):
        return self.__ROOT_ID;


    def getChildIDs(self, parentID):
        if(parentID.find(self.__PROJECT_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.BUILDING_PREFIX)

        elif(parentID.find(self.BUILDING_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.FLOOR_PREFIX)

        elif(parentID.find(self.FLOOR_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.ROOM_PREFIX)

        elif(parentID.find(self.BUILDING_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.FLOOR_PREFIX)

        elif(parentID.find(self.FLOOR_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.ROOM_PREFIX)

        elif(parentID.find(self.ROOM_PREFIX) > -1):
            return self.getIDList(self.getDataRootNode(parentID), self.JUNCTION_BOX_PREFIX)
        else:
            return None
#****************************************************************************
    def getParentID(self, childID):
        #child ID starts not with Project-Prefix
        if(childID.find(self.__PROJECT_PREFIX) == -1):

            ParentElement = self.getDataRootNode(childID).parentNode
            return ParentElement.getAttribute("id")
        else:
            return ""

#****************************************************************************
    #creates a new child-Element in the DOM and returns this Elements ID
    def addChild(self, parentID):

        parent = self.getDataRootNode(parentID)

        #print parentID
        #print self.getNodeName(parentID)

        if(parentID.find(self.__PROJECT_PREFIX) > -1):
            Element = self.createChild(self.BUILDING_PREFIX,1)

        elif(parentID.find(self.BUILDING_PREFIX) > -1):
            Element = self.createChild(self.FLOOR_PREFIX,1)

        elif(parentID.find(self.FLOOR_PREFIX) > -1):
            Element = self.createChild(self.ROOM_PREFIX,1)

        elif(parentID.find(self.ROOM_PREFIX) > -1):
            Element = self.createChild(self.JUNCTION_BOX_PREFIX,1)


        #topology elements
        if(parentID == self.TOPOLOGY_ROOT_ID):
            Element = self.createChild(self.TOPOLOGY_AREA,2)

        if(self.getNodeName(parentID) == self.TOPOLOGY_AREA):
            Element = self.createChild(self.TOPOLOGY_LINE,2)

        if(self.getNodeName(parentID) == self.TOPOLOGY_LINE):
            Element = self.createChild(self.TOPOLOGY_DEVICE,2)


        #groupadress elements
        if(parentID == self.GROUPADRESS_ROOT_ID):
            Element = self.createChild(self.GROUPADRESS_MAIN,2)

        if(self.getNodeName(parentID) == self.GROUPADRESS_MAIN):
            Element = self.createChild(self.GROUPADRESS_MIDDLE,2)

        if(self.getNodeName(parentID) == self.GROUPADRESS_MIDDLE):
            Element = self.createChild(self.GROUPADRESS,2)

        try:

            parent.appendChild(Element)
            ID = self.getChildID(Element)

        except:
            ID = None

        return ID

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
        self.setName(self.__ROOT_ID, unicode(name))
#****************************************************************************
    def getComment(self):
        Node = self.getDataRootNode(self.__ROOT_ID)
        return self.readDOMNodeValue(Node, "comment")

    def setComment(self, comment):
        Node = self.getDataRootNode(self.__ROOT_ID)
        self.writeDOMNodeValue(Node, "comment", unicode(comment))
#****************************************************************************
    def getPrefferedBusSystem(self):
        Node = self.getDataRootNode(self.__ROOT_ID)
        return self.readDOMNodeValue(Node, "preffered-bus-system")

    def setPrefferedBusSystem(self, pbs):
        Node = self.getDataRootNode(self.__ROOT_ID)
        self.writeDOMNodeValue(Node, "preffered-bus-system", unicode(pbs))
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
