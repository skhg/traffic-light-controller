#!/usr/bin/env python

import os

file_name = "public/index.html"
file_minified_name = "index_html_gz.h"

os.system("gzip --keep --best --force " + file_name)
file_name = file_name + ".gz"

os.system("xxd -i " + file_name + " > " + file_minified_name)
os.system("sed -i '' 's/unsigned/const/g' " + file_minified_name)
