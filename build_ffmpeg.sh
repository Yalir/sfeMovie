#!/bin/bash

os=""
cmake_env=""
linking=""
source_dir=""
build_dir=""
yasm_exe=""
temporary_dir="/tmp"
jobsCount=1 # how many compilations at a time
full_decoders_list=""
ffmpeg_dir="ffmpeg-2.8"
ffmpeg_archive="${ffmpeg_dir}.tar.bz2"


function check_err()
{
    if [ $? -ne 0 ]
      then
        echo "*** an error occured, aborting.";
        exit 1;
    fi
}

function build_ffmpeg()
{
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
    
    yasmpath="${yasm_exe}"

    if [ "$os" == "windows" ]
      then
        os_flags="--enable-memalign-hack --toolchain=msvc"
        # yasmpath=`echo "${yasmpath}" | sed -e 's_C:/_/C/_g' -e 's_D:/_/D/_g' -e 's_E:/_/E/_g' -e 's_F:/_/F/_g' -e 's_G:/_/G/_g'`
    fi
    $yasmpath --help > /dev/null
    check_err

    args="$args --disable-programs --disable-doc --disable-network --disable-decoders --disable-muxers --disable-encoders --yasmexe=\"${yasmpath}\" --enable-shared --disable-static $configure_flags $os_flags"
    
    chmod u+x "${temporary_ffmpeg_dir}/configure" "${temporary_ffmpeg_dir}/version.sh" "${temporary_ffmpeg_dir}/doc/texi2pod.pl"
    mkdir -p "${build_dir}/FFmpeg-objects"
    cd "${build_dir}/FFmpeg-objects"

    echo "${temporary_ffmpeg_dir}/configure $args"
    echo "Note: FFmpeg configuration may take some time, please be patient :)"
    { echo "$args" | xargs "${temporary_ffmpeg_dir}/configure"; }
    check_err
    make clean
    check_err
    make --jobs=$jobsCount
    check_err
    rm -rf "${temporary_ffmpeg_dir}"
    
    ffmpeg_sources_dir="${temporary_ffmpeg_dir}"
    ffmpeg_objects_dir="${build_dir}/FFmpeg-objects"
    ffmpeg_binaries_dir="${build_dir}/FFmpeg-binaries"

    mkdir -p "${ffmpeg_binaries_dir}/bin"
    mkdir -p "${ffmpeg_binaries_dir}/lib"
    rm -f "${ffmpeg_binaries_dir}/bin/*"
    rm -f "${ffmpeg_binaries_dir}/lib/*"
    
    echo "Copying libraries into ${ffmpeg_binaries_dir}"
    cd "${ffmpeg_objects_dir}"
    if [ "${os}" == "windows" ] ; then
        /usr/bin/find . -name '*.lib' -exec cp -v '{}' "${ffmpeg_binaries_dir}/lib" ';' 
        check_err
        /usr/bin/find . -name "*.dll" -exec cp -v '{}' "${ffmpeg_binaries_dir}/bin" ';'
        check_err
    else
        if [ "$os" == "linux" ]
          then
            # TODO: We don't want to follow symbolic links here
            find . -name "*.so*" -exec cp -vfl '{}' "${ffmpeg_binaries_dir}/lib" ';' 
        else
            # OS X
            find . -name "*.dylib" -exec cp -vR '{}' "${ffmpeg_binaries_dir}/lib" ';' 
        fi
        check_err

        if [ "$os" == "macosx" ] ; then
            cd "${ffmpeg_binaries_dir}/lib"
            "${source_dir}/install_names.sh"
        fi
    fi

    echo "Writing cache file into ${build_dir}/SelectedDecoders.cache"
    cmakeDecodersList=`echo ${full_decoders_list} | sed -e 's_ _;_g'`
    echo "${cmakeDecodersList}" > "${build_dir}/SelectedDecoders.cache"
    
    echo "Built ffmpeg"
}


function main()
{
    command_args="linux|windows|macosx jobsCount decoders_list"
    # want help?
    if [ "$1" == "-h" ] ||
       [ "$1" == "--help" ]
       then
         echo "Usage: $0 $command_args"
    else
        if ([ "$1" != "linux" ] && [ "$1" != "windows" ] && [ "$1" != "macosx" ])
          then
            echo "Usage: $0 $command_args"
            echo "Got $*"
            exit 1
        else
            # do build process
            os="$1"
            jobsCount="$2"
            
            if ! test -f "SourceDir.var" ; then
                cwd=`pwd`
                echo "Cannot find required file \"${cwd}/SourceDir.var\"! Exiting"
                exit 1
            fi

            if ! test -f "BuildDir.var" ; then
                cwd=`pwd`
                echo "Cannot find required file \"${cwd}/BuildDir.var\"! Exiting"
                exit 1
            fi

            if ! test -f "YasmPath.var" ; then
                cwd=`pwd`
                echo "Cannot find required file \"${cwd}/YasmPath.var\"! Exiting"
                exit 1
            fi

            source_dir=`cat SourceDir.var`
            build_dir=`cat BuildDir.var`
            yasm_exe=`cat YasmPath.var`

            shift
            shift
            full_decoders_list="$*"
            
            echo "Source directory : ${source_dir}"
            echo "Build directory  : ${build_dir}"
            echo "OS               : $os"
            echo "YASM executable  : ${yasm_exe}"
            echo "Decoders         : $full_decoders_list"
            
            build_ffmpeg $*
        fi
    fi
}

main $*

