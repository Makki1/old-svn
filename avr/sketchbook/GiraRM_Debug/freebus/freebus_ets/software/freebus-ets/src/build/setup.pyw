
# setup.py
from distutils.core import setup
import py2exe

import sys

sys.stdout = open('screen.txt','w',0)
sys.stderr = open('errors.txt','w',0)

setup(name='Freebus-ETS',
      version='1.0',
      author='Jerome Leisner',
      data_files=[('Image', ['Image\\ac0036-48.ico', 'Image\\building.png', 'Image\\floor.png', 'Image\\freebuslogo1bit.bmp', 'Image\\freebuslq5.png', 'Image\\junctionbox.png', 'Image\\New.gif', 'Image\\room.png', 'Image\\Thumbs.db'])],
      windows=[{'script':'Freebus-ETS.py',
                'icon_resources':[(1,'src\Image\ac0036-48.ico')],
                }])

print "---Done---"
