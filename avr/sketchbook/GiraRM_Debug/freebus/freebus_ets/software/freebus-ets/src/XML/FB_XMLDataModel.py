#!/usr/bin/
#-*- coding: iso-8859-1 -*-
#===============================================================================
#      __________  ________________  __  _______
#     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
#    / /_  / /_/ / __/ / __/ / __  / / / /\__ \
#   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
#  /_/   /_/ |_/_____/_____/_____/\____//____/
#
#Source File: FB_XMLDataModel.py
#Version: V0.1 , 22.01.2008
#Author: Jerome Leisner
#email: j.leisner@ing-automation.de
#===============================================================================

import xml.dom
#from xml.dom import dom
#from xml.dom.minidom import *


##general class for handling project data which are based on XML
class FB_XMLDataModel:

    __LogObj = None
    __DOMObj = None

    ##Constructor for an empty
    #@param LogObj: Log-File-Object to log all events within this inctance
    #@param projectname: Path and name of project
    def __init__(self,LogObj, Document, projectname):
        self.__LogObj = LogObj
        #create an DOMObj
        self.__DOMObj = Document


    ##return the current DOM-Object
    def getDOMObj(self):
        return self.__DOMObj

    ##sets a document
    def setDOMObj(self,document):
        self.__DOMObj = document

    #remove data with the given ID
    def removeData(self,ID):
        Node=self.getDataRootNode(ID)
        Parent = Node.parentNode
        Parent.removeChild(Node)

    #get the name of a Nodeelement
    def getName(self,ID):
        Node = self.getDataRootNode(ID)
        return self.readDOMNodeValue(Node, "name" )

    #set the name of Nodeelement
    def setName(self,ID,Name):
        Node = self.getDataRootNode(ID)
        self.writeDOMNodeValue(Node, "name", unicode(Name))

    #get comment
    def getComment(self, ID):
        Node = self.getDataRootNode(ID)
        return self.readDOMNodeValue(Node, "comment")

    #set comment
    def setComment(self,ID,Comment):
        Node = self.getDataRootNode(ID)
        self.writeDOMNodeValue(Node,"comment", unicode(comment))

    #get the NodeName
    def getNodeName(self,ID):
        Node = self.getDataRootNode(ID)
        return Node.tagName

    #set the adress of a structure element
    def setAdress(self,ID,Adress):
        Node = self.getDataRootNode(ID)
        self.writeDOMNodeValue(Node,"adress", unicode(Adress))

    #get the adress
    def getAdress(self,ID):

        Node = self.getDataRootNode(ID)
        return self.readDOMNodeValue(Node, "adress")

    ##Return the id list for all child nodes with namen name from the given stat node
    #@param node the start node
    #@param childname the child node name
    #@return the id String collection
    def getIDList(self,Node,ChildName):

        if(Node <> None):
            NodeList = Node.getElementsByTagName(ChildName)

            IDList = []
            for i in range(len(NodeList)):
                Element = NodeList.item(i)
                if(Element.hasAttribute("id")):
                    IDList.append(Element.getAttribute("id"))

            return IDList

        else:
            self.__LogObj.NewLog("Error at getIDList, Parameter Node is None",1)
            return None

    ##Create a new standard child data node. This node includes a name with id and a comment tag.
    #@param tagname the child root tag name
    #@param VarGroup: VarGroup contains a number 1 = only structure; 2 = with adress line; ...
    #@return the DOM Element
    def createChild(self,TagName, VarGroup):
        Element = self.__DOMObj.createElement(TagName)
        #creates the new ID (root-element name + a number (building + "-1" ...)
        id = TagName+"-" + str(self.getNewID(TagName))  # get id for the child

        Element.setAttribute("id", id)

        if(VarGroup == 1):
            Element.appendChild(self.__DOMObj.createElement("name"))
            Element.appendChild(self.__DOMObj.createElement("comment"))
        elif(VarGroup == 2):
            Element.appendChild(self.__DOMObj.createElement("name"))
            Element.appendChild(self.__DOMObj.createElement("comment"))
            Element.appendChild(self.__DOMObj.createElement("adress"))

        return Element

    ##@return the child id for the given child node
    def getChildID(self, ChildNode):
        return ChildNode.getAttribute("id")

    ##Returns an ID number for a new child node
    def getNewID(self, TagName):
        NodeList = self.__DOMObj.getElementsByTagName(TagName)

        # calculate maximum value
        max=0

        for i in range(len(NodeList)):
            Element = NodeList.item(i)
            if(Element.hasAttribute("id")):
                idstr = Element.getAttribute("id")

                prefix = TagName + "-"
                number = idstr.replace(prefix, "")

                if(int(number) > max):
                    max = int(number)
        return max + 1    # return maximum + 1


    ##Returns the root node with the given id
    def getDataRootNode(self,ID):
        drNode = None

        end = ID.find('-')         #find first character "-"

        NodeName = ""

        if(end <> -1):
            NodeName = ID[0: end]
        else:
            NodeName = ID

        NodeList = self.__DOMObj.getElementsByTagName(NodeName)

        #print NodeList

        if(len(NodeList) > 0):
            for i in range(len(NodeList)):

                Node = NodeList.item(i)

                #attr = Node.getAttribute("id")
                attr = Node.attributes

                if(attr <> None):
                    idNode = None
                    idNode = attr.getNamedItem("id")

                    #print idNode.nodeValue
                    if(idNode <> None):

                        if(idNode.nodeValue == ID):
                            drNode = Node
                            break

        return drNode

    #Writes the node value for the node given in the StringTokenizer path. If no such node exists it will created.
    #@param n path root node
    #@param path path to the node
    #@param value the value that will be written
    def writeDOMNodeValue(self, Node, Path, Value):
        #in case Node is None-Type, the parent Node is the root-node

        self.writeNodeValue(Node, Path, Value)

        self.__DOMObj.normalize()


    #Recursively DFS method for writing node values. If no such nod exists the method creates them.
    def writeNodeValue(self, Node, Path, Value):
        #there are no entries
        if(Node.childNodes == None):
            print "look at writeNodeValue..."
        #if(len(Node.childNodes) == 0):

        #    NewNode = Node.appendChild(self.__DOMObj.createElement(Path))
        #    print NewNode
            #self.writeNodeValue(NewNode, Path, Value)

        else:
            NodeList = Node.childNodes
            if(len(NodeList) > 0):
                found = False
                if(NodeList[0].nodeType == Node.ELEMENT_NODE):
                    for i in range(len(NodeList)):
                        #check if Node already exist
                        if(NodeList.item(i).nodeName == Path):
                            found = True
                            self.writeNodeValue(NodeList.item(i), Path, Value)
                            break

                        if(found == False):
                            if(Node.nodeType == Node.ELEMENT_NODE):
                                #print Node.parentNode
                                #Child = Node.documentElement.createElement(Path)
                                #NewNode = Node.appendChild(Child)
                                #self.writeNodeValue(NewNode, Path, Value)
                                pass
                #Nodetyp TEXT -> found correct level
                else:
                    Node.firstChild.data = Value
                    return
            #Len of NodeList = 0 -> writable Node doesnt exist
            else:

                if(Node.hasChildNodes == True):

                    if(Node.childNodes.item(0).nodeType == Node.TEXT_NODE):
                        Node.firstChild.data = Value
                        return
                    else:
                        Node.appendChild(self.__DOMObj.createTextNode(Value))
                        #unicode(Value,"ISO-8859-1")
                        return
                else:
                    Node.appendChild(self.__DOMObj.createTextNode(Value))
                    return


    ##BFS method for reading node values
    def readDOMNodeValue(self, Node, path):

        #NodeList contains all childnodes within Parent Node "Node"
        NodeList = Node.childNodes

        if(len(NodeList) > 0):
                cnt = 0
                for cnt in range(len(NodeList)):
                    if(NodeList.item(cnt).nodeName == path):
                        #get NodeObject with given path
                        actualVisited = NodeList.item(cnt)
                        break
        else:
            return ""

        #are there more subnodes of given path-node?
        if(actualVisited.hasChildNodes()):
            if(actualVisited.firstChild.nodeType == Node.TEXT_NODE):
                   Value = actualVisited.firstChild.data
                   return Value.encode("ISO-8859-1")
        else:
            return ""

        return ""