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
		install_name_tool -change /usr/local/lib/$sublib @rpath/$sublib -id @rapth/$dylib $dylib
	done
done
