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

os.system("xxd -i favicon_io/apple-touch-icon.png > apple_touch_icon_png.h")
os.system("xxd -i favicon_io/favicon.ico > favicon_ico.h")

os.system("sed -i '' 's/unsigned/const/g' apple_touch_icon_png.h")
os.system("sed -i '' 's/unsigned/const/g' favicon_ico.h")