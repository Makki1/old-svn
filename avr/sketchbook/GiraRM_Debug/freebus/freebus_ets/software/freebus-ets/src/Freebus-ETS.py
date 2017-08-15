#!/usr/bin/env python
#-*- coding: iso-8859-1 -*-
#!/usr/bin/
#===============================================================================
#Source File: Testaufruf.py
#Version: V0.1 vom 25.12.2007
#Autor: Jerome Leisner
#===============================================================================

#Path-structure
#Installation-Path
#    ->    Dist (name of directory of setup procedure
#    ->    Logging
#    -> GUI
#        -> Images

import sys
import pygtk
if not sys.platform == 'win32':
    pygtk.require('2.0')
import gtk



#from GUI.FB_MainFrame import * #MainFrame
#import src.GUI.FB_MainFrame

from GUI import FB_MainFrame

#import FB_MainFrame

if __name__ == '__main__':
    # enable threading
    gtk.gdk.threads_init()
    gtk.gdk.threads_enter()

    # create the main window
    FBMain = FB_MainFrame.MainFrame()

    # start the program loop
    FBMain.main()

    # cleanup
    gtk.gdk.threads_leave()

