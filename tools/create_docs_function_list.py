import xml.dom.minidom as XML
import sys
import os

# standard path assumes that this script is in OpenClonk/tools
directory = "../docs/sdk/script/fn";
if len(sys.argv) > 1:
	directory = sys.argv[1]
print("Working on " + directory)
# open output file at this point to write to when recursing the directory
output = open("functionlist.txt", "w+")
output.write("$functions = array(\n")

for dirname, dirnames, filenames in os.walk(directory):
	for filename in filenames:
		# only xml files
		if filename.find(".xml") == -1:
			continue;
		filename = dirname + "/" + filename
		print(filename)
		dom = XML.parse(filename);
		funcs = dom.getElementsByTagName("funcs")[0]
		func = funcs.getElementsByTagName("func")
		
		# no function? might be a constant!
		if not func or len(func) == 0:
			continue
		func = func[0]
		
		title = func.getElementsByTagName("title")[0].firstChild.nodeValue
		syntax = func.getElementsByTagName("syntax")[0]
		rtype = syntax.getElementsByTagName("rtype")[0].firstChild.nodeValue
		
		# write "\t'int Abs ("
		output.write("\t'" + rtype + " " + title + " (")
		
		params = syntax.getElementsByTagName("params")
		if not params or len(params) == 0:
			pass
		else:
			firstpar = True
			params = params[0]
			for param in params.getElementsByTagName("param"):
				type = param.getElementsByTagName("type")
				
				# might be a "..." parameter? See FindObject.xml
				if not type or len(type) == 0 or not type[0].firstChild:
					type = ""
				else:
					type = type[0].firstChild.nodeValue + " "
				
				name = param.getElementsByTagName("name")[0].firstChild.nodeValue
				
				if not firstpar:
					output.write(", ")
				firstpar = False
				output.write(type + name);
		output.write(")',\n");
			
output.write(");\n")
output.close()

print("DONE!")