#!/usr/bin/python
# coding=utf-8
# 22.12.2011
# Dieses Programm kommuniziert mit Gira RWM

import serial
import signal
import sys
import binascii
import types
import time

# Seriellen Port eintragen
#ser = serial.Serial('/dev/ttyUSB0', 9600,)
ser = serial.Serial('/dev/ttyACM0', 9600,)

# Steuerzeichen
null = binascii.unhexlify("00")
stx  = binascii.unhexlify("02")
etx  = binascii.unhexlify("03")
ack  = binascii.unhexlify("06")

# Daten im Format [Req, Res] 
req_res = [ 
[stx+"0464"+etx,""],
[stx+"0868"+etx,""],
[stx+"0969"+etx,""],
[stx+"0B72"+etx,""],
[stx+"0C73"+etx,""],
[stx+"0D74"+etx,""],
[stx+"0E75"+etx,""]]

result = []

# Sauber Beenden mit Strg+C
def process_sigint(signum, frame):
	print "Ende"
	ser.close()
	sys.exit(0)

# Daten lesen
def get_data(p_req_res):
	state = 0
	data = ""
	for rr in p_req_res:
#		print "Sent Data ",rr[0]
		ser.flushInput()
		ser.write(rr[0])
		while 1:
			read = ser.read()
			if read == null or read == ack:
#				print "Got 1Data ", read, "->", binascii.b2a_hex(read)
				continue
			if read == stx:
		#		print "Begin"
				state = 1
				data = ""
			if read == etx:
				state = 0
				data = data + read
#				print "Got Data ", data, "->", binascii.b2a_hex(data)
				rr[1] = data
				ser.write(ack)
				time.sleep(0.1)
				break
			if state == 1:
				data = data + read

# Daten auswerten
def process_data(p_req_res):
	n = 0
	for rr in p_req_res:
		item = ["",""]
		n=n+1
		if n == 1:
			item = ["Serial",int(rr[1][3:11],16)]
		elif n == 2:
			item = ["?",int(rr[1][3:11],16)]
		elif n == 3:
			item = ["Betriebszeit",int(rr[1][3:11],16)/4/60/60]
		elif n == 4:
			item = ["Rauchkammer",int(rr[1][3:7],16)*0.003223]
			result.append(item)
			item = ["Anzahl Rauchalarm",int(rr[1][7:9],16)]
			result.append(item)
			item = ["Verschmutzungsgrad",int(rr[1][9:11],16)]
		elif n == 5:
			item = ["Spannung Batterie",int(rr[1][3:7],16)*0.018369]
			result.append(item)
			item = ["Temp 1",(int(rr[1][7:9],16)/2)-20]
			result.append(item)
			item = ["Temp 2",(int(rr[1][9:11],16)/2)-20]
		elif n == 6:
			item = ["Anzahl Thermoalarm",int(rr[1][3:5],16)]
			result.append(item)
			item = ["Anzahl Testalarm",int(rr[1][5:7],16)]
			result.append(item)
			item = ["Anzahl Alarm Draht",int(rr[1][7:9],16)]
			result.append(item)
			item = ["Anzahl Alarm Funk",int(rr[1][9:11],16)]
		elif n == 7:
			item = ["Anzahl Testalarm Draht",int(rr[1][3:5],16)]
			result.append(item)
			item = ["Anzahl Testalarm Funk",int(rr[1][5:7],16)]
		result.append(item)

		


signal.signal(signal.SIGINT, process_sigint)


get_data(req_res)

#for rr in req_res:			
#	print rr[0], "-->",rr[1], "-->",binascii.b2a_hex(rr[1])

process_data(req_res)
for res in result:
	print res[0], "-->",res[1]


