#! /usr/bin/env python

import fnmatch
import os
import sys
import re


###############
## functions ##
###############


# converts a sound path to a name string
def sound_path_to_name(sound_path):
	sound_name = os.path.splitext(os.path.basename(sound_path))[0]
	dir_name = os.path.dirname(sound_path)
	# sound is inside an object definition: return DEF::NAME
	if fnmatch.fnmatch(dir_name, '*.ocd'):
		return get_namespace_ocd(dir_name) + sound_name
	# sound is inside a .ocg folder: return FOLDER::NAME
	if fnmatch.fnmatch(dir_name, '*.ocg'):
		return get_namespace_ocg(dir_name) + sound_name
	# sound is inside a .ocs or .ocs folder: return NAME
	if fnmatch.fnmatch(dir_name, '*.ocs') or fnmatch.fnmatch(dir_name, '*.ocf'):
		return sound_name
	# return a warning, this should not happen
	return "Warning: sound found in illegal location (i.e. not in scenario, scenario folder, Sound.ocg folder or subfolerd, object definition."

# gets the sound namespace of object definition
def get_namespace_ocd(ocd_path):
	if not os.path.isfile(os.path.join(ocd_path, "DefCore.txt")):
		return "NOTFOUND::"
	with open(os.path.join(ocd_path, "DefCore.txt"), "r") as read_defcore:
		for line in read_defcore:
			if "id=" in line:
				return str(line).split('=')[-1].replace('\n', '') + "::"
	return "NOTFOUND::"

# gets the sound namespace of an ocg folder
def get_namespace_ocg(ocg_path):
	dirname = ocg_path
	namespace = ""
	while not fnmatch.fnmatch(dirname, '*Sound.ocg') and not fnmatch.fnmatch(dirname, '*Music.ocg'):
		basename = os.path.basename(dirname)
		dirname = os.path.dirname(dirname)
		namespace = os.path.splitext(basename)[0] + "::" + namespace		
	return namespace

# finds all sound calls in a script file
def find_sound_calls_script(script_file):
	with open(script_file, "r") as read_script:
		script = read_script.read()
	# regular call: Sound("FooBar")
	calls = re.findall('Sound\(\s*.*\)', script)
	calls = (str(re.sub('Sound\(\s*"', '', call).split('"')[0]) for call in calls)
	# regular call: SoundAt("FooBar")
	callsat = re.findall('SoundAt\(\s*.*\)', script)
	callsat = (str(re.sub('SoundAt\(\s*"', '', call).split('"')[0]) for call in callsat)
	# ActMap property: Sound = "FooBar"
	actprops = re.findall('Sound\s*=\s*.*\n', script)
	actprops = (str(re.sub('Sound\s*=\s*"', '', prop).split('"')[0]) for prop in actprops)
	all_calls = []
	for call in calls:
		all_calls.append(call)
	for call in callsat:
		all_calls.append(call)
	for prop in actprops:
		all_calls.append(prop)
	return all_calls

# finds all sound calls in a source file
def find_sound_calls_source(source_file):
	with open(source_file, "r") as read_source:
		source = read_source.read()
	calls = re.findall('GUISound\(.*\)|StartSoundEffectAt\(.*\)|StartSoundEffect\(.*\)', source)
	calls = (call.replace('GUISound("', '').replace('StartSoundEffectAt("', '').replace('StartSoundEffect("', '').split('"')[0] for call in calls)
	return calls


#############
## program ##
#############


# program arguments: planet directory and source directory
planet_dir = "../planet"
source_dir = "../src"
if len(sys.argv) >= 2:
	planet_dir = sys.argv[1]
if len(sys.argv) >= 3:
	source_dir = sys.argv[2]

# find all sound and script files in planet
sound_files = []
script_files = []
for root, directories, filenames in os.walk(planet_dir):
	for filename in filenames:
		full_filename = os.path.join(root, filename)
		# get the sound files
		if fnmatch.fnmatch(full_filename, '*.ogg') or fnmatch.fnmatch(full_filename, '*.wav'):
			sound_files.append(full_filename)
		# get the script files
		if fnmatch.fnmatch(full_filename, '*.c'):
			script_files.append(full_filename)

# remove music files from sound files
sound_files[:] = (sound_path for sound_path in sound_files if not fnmatch.fnmatch(sound_path, '*Music.ocg*'))

# find all engine files in source
source_files = []
for root, directories, filenames in os.walk(source_dir):
	for filename in filenames:
		full_filename = os.path.join(root, filename)
		# get the source files		
		if fnmatch.fnmatch(full_filename, '*.cpp') or fnmatch.fnmatch(full_filename, '*.h'):
			source_files.append(full_filename)


# get all sound calls from the script and the source files
sound_calls_script = []
for script in script_files:
	for call in find_sound_calls_script(script):
		sound_calls_script.append(call)
sound_calls_source = []
for source in source_files:
	for call in find_sound_calls_source(source):
		sound_calls_source.append(call)

# check for all found sound files if there is an occurence in the script
print "Looking for unused sounds in the planet directory: \n\t", planet_dir
print "WARNING: always double check if the sound really is not played by searching for the sound name in all engine and script files."
print "\nChecking", len(sound_files), "sound files ..."
cnt_unused_sounds = 0
for sound_path in sound_files:
	sound = sound_path_to_name(sound_path)
	found = False
	for call in sound_calls_script + sound_calls_source:
		if fnmatch.fnmatch(sound, call):	
			found = True
			break
	if not found:
		cnt_unused_sounds += 1
		print sound_path + " (" + sound + ")" + " has not been found"
print "Out of the", len(sound_files), "found sound files, the above", cnt_unused_sounds, "may be unused."



# script for finding non-normal calls to sound only turn on if needed
if False:
	for script_file in script_files:
		with open(script_file, "r") as read_script:
			script = read_script.read()
		calls = re.findall('Sound\(\s*.*\)', script)
		for call in calls:
			if re.search('Sound\(\s*"', call) is None:
				print call
		calls = re.findall('SoundAt\(\s*.*\)', script)
		for call in calls:
			if re.search('SoundAt\(\s*"', call) is None:
				print call
		calls = re.findall('Sound\s*=\s*.*', script)
		for call in calls:
			if re.search('Sound\s*=\s*".*".*', call) is None:
				print call

