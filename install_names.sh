#!/bin/sh

files=`ls *.dylib`
dylibs=""

for file in $files ; do
	if ! test -h "$file" ; then
		dylibs="$dylibs $file"
	fi
done

for dylib in $dylibs ; do
	for sublib in $dylibs ; do
		install_name_tool -change /usr/local/lib/$sublib @loader_path/$sublib -id @loader_path/Libraries/$dylib $dylib
	done
done
