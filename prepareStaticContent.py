#!/usr/bin/env python

import os

content_file_names = ["index.html", "app.js", "style.css"]

for file_name in content_file_names:
	fin = open(file_name, "rt")

	file_minified_name = file_name.replace('.','_')

	fout = open(file_minified_name + ".h", "wt")

	singleLine = "";

	for line in fin:
		singleLine+=line.replace('\n','')

	file_minified_name
	fout.write("const String " + file_minified_name.upper() + " = \"")
	fout.write(singleLine)
	fout.write("\";")

image_file_names = ["favicon_io/apple-touch-icon.png", "favicon_io/favicon.ico"]

for image_file_name in image_file_names:
	header_file_name = image_file_name.replace('favicon_io/','').replace('.','_').replace('-','_') + ".h"

	os.system("xxd -i " +image_file_name + " > " + header_file_name)
	os.system("sed -i '' 's/unsigned/const/g' " + header_file_name)
