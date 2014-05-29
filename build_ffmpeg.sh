#!/bin/bash

os=""
cmake_env=""
linking=""
source_dir=""
build_dir=""
temporary_dir="/tmp"
macosx_sdk=""
vcpp=0
jobsCount=1 # how many compilations at a time
full_decoders_list=""
ffmpeg_dir="ffmpeg-2.2.2"
ffmpeg_archive="${ffmpeg_dir}.tar.bz2"
yasm_dir="yasm-1.2.0"
yasm_archive="${yasm_dir}.tar.gz"


function check_err()
{
	if [ $? -ne 0 ]
	  then
	    echo "*** an error occured, aborting.";
	    exit 1;
	fi
}

function build_yasm()
{
	cd "${build_dir}"
	temporary_yasm_dir="${temporary_dir}/${yasm_dir}"

	if test -d "${temporary_yasm_dir}" ; then
    	rm -rf "${temporary_yasm_dir}"
    fi

    if test -f "${source_dir}/FFmpeg/${yasm_archive}" ; then
		echo "Extracting YASM archive..."

		# On Windows, MinGW's tar fails at resolving paths starting with "C:/", thus we replace the beginning with "/C/" which works fine and
		# won't affect others OSs in most cases (NB: we also handle the cases where the root disk is not C)
		src=`echo "${source_dir}/FFmpeg/${yasm_archive}" | sed -e 's_C:/_/C/_g' -e 's_D:/_/D/_g' -e 's_E:/_/E/_g' -e 's_F:/_/F/_g' -e 's_G:/_/G/_g'`
		echo "tar -C \"${temporary_dir}\" -xjf \"${src}\""
		tar -C "${temporary_dir}" -xzf "${src}"
		check_err
	else
		echo "Cannot find YASM archive at ${source_dir}/FFmpeg/${yasm_archive}"
		exit 1
	fi

	chmod u+x "${temporary_yasm_dir}/configure"
	mkdir -p "${build_dir}/YASM-objects"
    cd "${build_dir}/YASM-objects"

    echo "${temporary_yasm_dir}/configure"
    "${temporary_yasm_dir}/configure"
    check_err
    make clean
    check_err
    make --jobs=$jobsCount
    check_err

    rm -rf "${temporary_yasm_dir}"
}

function build_ffmpeg()
{
	build_yasm

	cd "${build_dir}"
	
    # build ffmpeg
    temporary_ffmpeg_dir="${temporary_dir}/${ffmpeg_dir}"

    if test -d "${temporary_ffmpeg_dir}" ; then
    	rm -rf "${temporary_ffmpeg_dir}"
    fi

	if test -f "${source_dir}/FFmpeg/${ffmpeg_archive}" ; then
		echo "Extracting FFmpeg archive..."

		# On Windows, MinGW's tar fails at resolving paths starting with "C:/", thus we replace the beginning with "/C/" which works fine and
		# won't affect others OSs in most cases (NB: we also handle the cases where the root disk is not C)
		src=`echo "${source_dir}/FFmpeg/${ffmpeg_archive}" | sed -e 's_C:/_/C/_g' -e 's_D:/_/D/_g' -e 's_E:/_/E/_g' -e 's_F:/_/F/_g' -e 's_G:/_/G/_g'`
		echo "tar -C \"${temporary_dir}\" -xjf \"${src}\""
		tar -C "${temporary_dir}" -xjf "${src}"
		check_err
	else
		echo "Cannot find FFmpeg archive at ${source_dir}/FFmpeg/${ffmpeg_archive}"
		exit 1
	fi

    configure_flags="";
    
    for codec in $full_decoders_list
      do
        configure_flags="$configure_flags --enable-decoder=$codec"
    done
    
    if [ "$os" == "macosx" ]
      then
      	if [ "$macosx_sdk" != "" ]
      	  then
      	    os_flags="$os_flags --sysroot=$macosx_sdk"
      	fi	
    fi
	
	yasmpath="${build_dir}/YASM-objects/yasm"

	if [ "$os" == "windows" ]
	  then
		os_flags="--enable-memalign-hack --enable-pthreads"
		yasmpath=`echo "${yasmpath}.exe" | sed -e 's_C:/_/C/_g' -e 's_D:/_/D/_g' -e 's_E:/_/E/_g' -e 's_F:/_/F/_g' -e 's_G:/_/G/_g'`
	fi

	args="$args --disable-stripping --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-doc --disable-network --disable-decoders --disable-muxers --disable-encoders --yasmexe=${yasmpath} --enable-shared --disable-static $configure_flags $os_flags"

	#setup VC++ env variables to find lib.exe
	if [ "$vcpp" == "1" ]
	  then
		old_path=`echo $PATH`
		export PATH="$PATH:/C/Program Files/Microsoft Visual Studio 9.0/Common7/IDE:/C/Program Files/Microsoft Visual Studio 9.0/VC/bin"
		export PATH="$PATH:/C/Program Files/Microsoft Visual Studio 10.0/Common7/IDE:/C/Program Files/Microsoft Visual Studio 10.0/VC/bin"
		export PATH="$PATH:/C/Program Files/Microsoft Visual Studio 11.0/Common7/IDE:/C/Program Files/Microsoft Visual Studio 11.0/VC/bin"
		export PATH="$PATH:/C/Program Files (x86)/Microsoft Visual Studio 9.0/Common7/IDE:/C/Program Files (x86)/Microsoft Visual Studio 9.0/VC/bin"
		export PATH="$PATH:/C/Program Files (x86)/Microsoft Visual Studio 10.0/Common7/IDE:/C/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin"
		export PATH="$PATH:/C/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE:/C/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin"
	fi
	
    chmod u+x "${temporary_ffmpeg_dir}/configure" "${temporary_ffmpeg_dir}/version.sh" "${temporary_ffmpeg_dir}/doc/texi2pod.pl"
    mkdir -p "${build_dir}/FFmpeg-objects"
    cd "${build_dir}/FFmpeg-objects"

    echo "${temporary_ffmpeg_dir}/configure $args"
    { echo "$args" | xargs "${temporary_ffmpeg_dir}/configure"; }
    check_err
    make clean
    check_err
    make --jobs=$jobsCount
    check_err
    rm -rf "${temporary_ffmpeg_dir}"
    
    if [ "$vcpp" == "1" ]
      then
		export PATH="$old_path"
	fi
    
    ffmpeg_sources_dir="${temporary_ffmpeg_dir}"
    ffmpeg_objects_dir="${build_dir}/FFmpeg-objects"
    ffmpeg_binaries_dir="${build_dir}/FFmpeg-binaries"

	mkdir -p "${ffmpeg_binaries_dir}/bin"
	mkdir -p "${ffmpeg_binaries_dir}/lib"
	rm -f "${ffmpeg_binaries_dir}/bin/*"
	rm -f "${ffmpeg_binaries_dir}/lib/*"
    
	echo "Copying libraries into ${ffmpeg_binaries_dir}"
	if [ "$vcpp" == "1" ]
	  then
		find "${ffmpeg_objects_dir}" -name "*.lib" -exec cp -v '{}' "${ffmpeg_binaries_dir}/lib" ';' 
		check_err
		find "${ffmpeg_objects_dir}" -name "*.dll" -exec cp -v '{}' "${ffmpeg_binaries_dir}/bin" ';'
		check_err
	else
		if [ "$os" == "linux" ]
		  then
		    find "${ffmpeg_objects_dir}" -name "*.so*" -exec cp -vfl '{}' "${ffmpeg_binaries_dir}/lib" ';' 
		else
			find "${ffmpeg_objects_dir}" -name "*.dylib" -exec cp -vR '{}' "${ffmpeg_binaries_dir}/lib" ';' 
		fi
		check_err

		if [ "$os" == "macosx" ] ; then
			cd "${ffmpeg_binaries_dir}/lib"
			"${source_dir}/install_names.sh"
		fi
	fi
	
	echo "Built ffmpeg"
}


function main()
{
	command_args="linux|windows|macosx novs|vs decoders_list"
	# want help?
	if [ "$1" == "-h" ] ||
	   [ "$1" == "--help" ]
	   then
	     echo "Usage: $0 $command_args"
	else
		if ([ "$1" != "linux" ] && [ "$1" != "windows" ] && [ "$1" != "macosx" ]) ||
			([ "$2" != "novs" ] && [ "$2" != "vs" ])
		  then
		    echo "Usage: $0 $command_args"
		    echo "Got $*"
		    exit 1
		else
			# do build process
			os="$1"
			
			if [ "$2" == "vs" ]
			  then
				vcpp="1"
			fi
			
			source_dir=`cat SourceDir.var`
			build_dir=`cat BuildDir.var`

			if ! [ "$os" == "windows" ] ; then
				jobsCount=5
			fi

			shift
			shift
			full_decoders_list="$*"
			
			echo "Build directory : ${build_dir}"
			echo "OS              : $os"
			echo "Visual Studio   : $vcpp"
			echo "Decoders        : $full_decoders_list"
			
			build_ffmpeg $*
		fi
	fi
}

main $*

