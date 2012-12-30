import os
import os.path
import glob

def search_all_files_in_directory_for_phrase(parentdir, searchphrase, exclude_files):
	for dirname, dirnames, filenames in os.walk(parentdir):
		for filename in filenames:
			fullfilename = os.path.normpath(os.path.join(dirname, filename))
			if fullfilename in exclude_files:
				continue
			with open(fullfilename, "r") as searchfile:
				for line in searchfile:
					if searchphrase in line:
						return True
	return False

language_files = glob.glob(os.path.normpath("../planet/System.ocg/Language??.txt"))

for language_file in language_files:

	print language_file

	fout = open( language_file + "new" , "w") 
	
	with open(language_file,"r") as file:
	
		lineno = 0
		unused = []
	
		for line in file:

			lineno+=1
			print "Checking line " + str(lineno) + "\r",
			
			keyvalue = line.split("=")
			if len(keyvalue) < 2:
				continue
			key = keyvalue[0].strip()
			findkey = key.rstrip("0123456789")
			
			if not search_all_files_in_directory_for_phrase(os.path.normpath("../src"),findkey,language_files) \
			and not search_all_files_in_directory_for_phrase(os.path.normpath("../planet"),findkey,language_files):
				unused.append(key)
			else:
				fout.write(line)
				fout.flush()

		print "removing "
		for field in unused:
			print "  "+field
			
	fout.close()
	
	os.remove(language_file)
	os.rename(language_file + "new",language_file)