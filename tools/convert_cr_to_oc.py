#!/usr/bin/python

import os
import re

namere = re.compile("([^=]+)=(.+)");
langre = re.compile("(..):(.+)");
localre = re.compile("local (\w+) ?= ?(.+);");
idre = re.compile("[A-Z0-9_][A-Z0-9_][A-Z0-9_][A-Z0-9_]")

def convertname(root, files, name):
	names = {}
	r = "\"" + name + "\""
	if "Names.txt" in files:
		f = open(os.path.join(root, "Names.txt"),"Ur")
		for line in f:
			m = langre.match(line)
			if m:
				names[m.group(1)] = m.group(2)
		f.close()
		os.unlink(os.path.join(root, "Names.txt"))
		r = "\"$Name$\""
	else:
		print root, name
	for lang, localname in names.iteritems():
		if "StringTbl" + lang + ".txt" in files:
			lines = open(os.path.join(root, "StringTbl" + lang + ".txt"),"rb").read().splitlines()
		else:
			lines = ()
		f = open(os.path.join(root, "StringTbl" + lang + ".txt"),"wb")
		for line in lines:
			f.write(line)
			f.write("\r\n")
		f.write("Name=" + localname + "\r\n")
	return r

def convertactmap(root, files):
	lines = open(os.path.join(root, "ActMap.txt"),"rb").read().splitlines()
	actmap = "{\r\n"
	act = ""
	for line in lines:
		if line.find("[Action]") > -1:
			if act != "":
				actmap = actmap + act + "},\r\n"
			act = ""
			continue
		m = namere.match(line)
		if m and m.group(1) == "Name":
			if idre.match(m.group(2)):
				act = "\"" + m.group(2) + "\" = {\r\nPrototype = Action,\r\n"
			else:
				act = m.group(2) + " = {\r\nPrototype = Action,\r\n"
		if m and m.group(1) == "Facet":
			facet = m.group(2).split(",")
			if len(facet) > 0: act = act + "X = " + facet[0] + ",\r\n"
			if len(facet) > 1: act = act + "Y = " + facet[1] + ",\r\n"
			if len(facet) > 2: act = act + "Wdt = " + facet[2] + ",\r\n"
			if len(facet) > 3: act = act + "Hgt = " + facet[3] + ",\r\n"
			if len(facet) > 4: act = act + "OffX = " + facet[4] + ",\r\n"
			if len(facet) > 5: act = act + "OffY = " + facet[5] + ",\r\n"
		elif m and m.group(1) in ("Directions", "FlipDir", "Length", "Attach", "Delay",
			"FacetBase", "FacetTopFace", "FacetTargetStretch", "NoOtherAction",
			"ObjectDisabled", "DigFree","EnergyUsage", "Reverse", "Step"):
			act = act + m.group(1) + " = " + m.group(2) + ",\r\n"
		elif m and m.group(1) == "Procedure":
			act = act + m.group(1) + " = DFA_" + m.group(2).strip() + ",\r\n"
		elif m and m.group(1) in ("StartCall", "EndCall", "PhaseCall", "AbortCall"):
			if m.group(2) != "None":
				act = act + m.group(1) + " = \"" + m.group(2).strip() + "\",\r\n"
		elif m:
			act = act + m.group(1) + " = \"" + m.group(2) + "\",\r\n"
	if act != "":
		actmap = actmap + act + "}, "
	os.unlink(os.path.join(root, "ActMap.txt"))
	return actmap + " }"
	

for root, dirs, files in os.walk('.'):
	if not "DefCore.txt" in files:
		continue
	lines = open(os.path.join(root, "DefCore.txt"),"Ur").read().splitlines()
	f = open(os.path.join(root, "DefCore.txt"),"w")
	properties = {}
	for line in lines:
		m = namere.match(line)
		if m and m.group(1) == "Name":
			properties["Name"] = convertname(root, files, m.group(2))
		elif m and m.group(1) == "Collectible":
			properties["Collectible"] = m.group(2)
		elif m and m.group(1) == "Grab":
			properties["Touchable"] = m.group(2)
		else:
			f.write(line)
			f.write("\n")	
	if "ActMap.txt" in files:
		properties["ActMap"] = convertactmap(root, files)
	try:
		f = open(os.path.join(root, "Script.c"),"r+b")
		lines = f.read().splitlines()
	except:
		lines = ("")
	f = open(os.path.join(root, "Script.c"),"wb")
	for i, line in enumerate(lines):
		m = localre.match(line)
		if m:
			if properties.get(m.group(1)):
				print root, m.group(1), properties.get(m.group(1)), m.group(2)
			properties.pop(m.group(1), None)
		f.write(line)
		f.write("\n")
	for prop, value in properties.iteritems():
		f.write("local " + prop + " = " + value + ";\n")

