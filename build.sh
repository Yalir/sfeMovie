#!/bin/bash

os=""
cmake_env=""
linking=""
macosx_arch="x86_64" # ppc or i386 or x86_64
macosx_sdk=""
has_vda="0"
vcpp=0
use_cache=1
jobsCount=4 # how many compilations at a time
codec_list="aac aac_latm aasc ac3 adpcm_4xm adpcm_adx adpcm_ct adpcm_ea adpcm_ea_maxis_xa adpcm_ea_r1 adpcm_ea_r2 adpcm_ea_r3 adpcm_ea_xas adpcm_g722 adpcm_g726 adpcm_ima_amv adpcm_ima_dk3 adpcm_ima_dk4 adpcm_ima_ea_eacs adpcm_ima_ea_sead adpcm_ima_iss adpcm_ima_qt adpcm_ima_smjpeg adpcm_ima_wav adpcm_ima_ws adpcm_ms adpcm_sbpro_2 adpcm_sbpro_3 adpcm_sbpro_4 adpcm_swf adpcm_thp adpcm_xa adpcm_yamaha aea aiff alac als amr amrnb amrwb amv anm ansi apc ape applehttp asf ass asv1 asv2 atrac1 atrac3 au aura aura2 avi avisynth avs bethsoftvid bfi bink binkaudio_dct binkaudio_rdft bmp c93 caf cavs cavsvideo cdg cdgraphics cinepak cljr cook cscd cyuv daud dca dfa dirac dnxhd dpx dsicin dsicinaudio dsicinvideo dts dv dvbsub dvdsub dvvideo dxa ea ea_cdata eac3 eacmv eamad eatgq eatgv eatqi eightbps eightsvx_exp eightsvx_fib eightsvx_raw escape124 ffm ffmetadata ffv1 ffvhuff filmstrip flac flashsv flic flv fourxm fraps frwu g722 gif gsm gsm_ms gxf h261 h263 h263i h264 h264_crystalhd h264_vdpau huffyuv idcin iff iff_byterun1 iff_ilbm image2 image2pipe imc indeo2 indeo3 indeo5 ingenient interplay_dpcm interplay_video ipmovie iss iv8 ivf jpeg2000 jpegls jv kgv1 kmvc lagarith libcelt libdirac libgsm libgsm_ms libnut libopencore_amrnb libopencore_amrwb libopenjpeg libschroedinger libspeex libvpx lmlm4 loco lxf m4v mace3 mace6 matroska mdec microdvd mimic mjpeg mjpegb mlp mm mmf mmvideo motionpixels mov mp1 mp1float mp2 mp2float mp3 mp3adu mp3adufloat mp3float mp3on4 mp3on4float mpc mpc7 mpc8 mpeg1_vdpau mpeg1video mpeg2_crystalhd mpeg2video mpeg4 mpeg4_crystalhd mpeg4_vdpau mpeg_vdpau mpeg_xvmc mpegps mpegts mpegtsraw mpegvideo msmpeg4_crystalhd msmpeg4v1 msmpeg4v2 msmpeg4v3 msnwc_tcp msrle msvideo1 mszh mtv mvi mxf mxg mxpeg nc nellymoser nsv nut nuv ogg oma pam pbm pcm_alaw pcm_bluray pcm_dvd pcm_f32be pcm_f32le pcm_f64be pcm_f64le pcm_lxf pcm_mulaw pcm_s16be pcm_s16le pcm_s16le_planar pcm_s24be pcm_s24daud pcm_s24le pcm_s32be pcm_s32le pcm_s8 pcm_u16be pcm_u16le pcm_u24be pcm_u24le pcm_u32be pcm_u32le pcm_u8 pcm_zork pcx pgm pgmyuv pgssub pictor pmp png ppm ptx pva qcelp qcp qdm2 qdraw qpeg qtrle r10k r210 r3d ra_144 ra_288 rawvideo rl2 rm roq roq_dpcm rpl rpza rso rtp rtsp rv10 rv20 rv30 rv40 s302m sap sdp segafilm sgi shorten siff sipr smackaud smacker smc snow sol sol_dpcm sonic sox sp5x spdif srt str sunrast svq1 svq3 swf targa theora thp tiertexseq tiertexseqvideo tiff tmv truehd truemotion1 truemotion2 truespeech tscc tta tty twinvq txd ulti v210 v210x vb vc1 vc1_crystalhd vc1_vdpau vc1t vcr1 vmd vmdaudio vmdvideo vmnc voc vorbis vp3 vp5 vp6 vp6a vp6f vp8 vqa vqf w64 wav wavpack wc3 wmapro wmav1 wmav2 wmavoice wmv1 wmv2 wmv3 wmv3_crystalhd wmv3_vdpau wnv1 ws_snd1 wsaud wsvqa wtv wv xa xan_dpcm xan_wc3 xan_wc4 xl xsub xwma yop yuv4mpegpipe zlib zmbv "

function check_err()
{
	if [ $? -ne 0 ]
	  then
	    echo "*** an error occured, aborting.";
	    exit 1;
	fi
}

function setup()
{
	if [ "$os" == "macosx" ]
      then
      	# pre Xcode 4.3 SDKs
      	if test -d "/Developer/SDKs/MacOSX10.5.sdk"
      	  then
      	    macosx_105_sdk="/Developer/SDKs/MacOSX10.5.sdk"
      	    has_105=1
      	fi
  		if test -d "/Developer/SDKs/MacOSX10.6.sdk"
  		  then
  		    macosx_106_sdk="/Developer/SDKs/MacOSX10.6.sdk"
  		    has_106=1
  		fi
		if test -d "/Developer/SDKs/MacOSX10.7.sdk"
		  then
		    macosx_107_sdk="/Developer/SDKs/MacOSX10.7.sdk"
		    has_107=1
		fi
		# Xcode 4.3 and later SDKs
		if test -d "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk"
		  then
		    macosx_106_sdk="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk";
		    has_106=1
		fi
		if test -d "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk"
		  then
		    macosx_107_sdk="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk";
		    has_107=1
		fi
    
		if [ "$has_105" == "" ] && [ "$has_106" == "" ] && [ "$has_107" == "" ]
		  then
			echo "Could not find any SDK. Sources will be built without specifying any SDK."
		else
			echo "Choose the Mac OS X SDK you want to link against (default is 0):"
			echo "0. None (use current OS headers)"
			
			if [ "$has_105" == "1" ]
			  then
				echo "5. Mac OS X 10.5 SDK"
			fi
			
			if [ "$has_106" == "1" ]
			  then
				echo "6. Mac OS X 10.6 SDK"
			fi
			
			if [ "$has_107" == "1" ]
			  then
				echo "7. Mac OS X 10.7 SDK"
			fi
			
			read choice
			
			if [ "$choice" == "5" ] && [ "$has_105" == "1" ]
			  then
				macosx_sdk=$macosx_105_sdk
			elif [ "$choice" == "6" ] && [ "$has_106" == "1" ]
			  then
				macosx_sdk=$macosx_106_sdk
			elif [ "$choice" == "7" ] && [ "$has_107" == "1" ]
			  then
				macosx_sdk=$macosx_107_sdk
				has_vda=1
			fi
			
			if [ "$macosx_sdk" != "" ]
			  then
				echo "Selected SDK: $macosx_sdk"
			fi
		fi
	fi
}

function build_ffmpeg()
{
	tenv=""
	if [ "$os" == "windows" ]
	  then
		echo "Choose your target environment (default is 1):"
		echo "1. MinGW"
		echo "2. Visual Studio 2005"
		echo "3. Visual Studio 2008"
		echo "4. Visual Studio 2010"
		echo "5. Other"
		echo ""
		
		read tenv
		
		if [ "$tenv" == "" ] || [ "$tenv" == "1" ]
		  then
			cmake_env="MSYS Makefiles"
		elif [ "$tenv" == "2" ]
		  then
			cmake_env="Visual Studio 8 2005"
			vcpp=1
		elif [ "$tenv" == "3" ]
		  then
			cmake_env="Visual Studio 9 2008"
			vcpp=1
		elif [ "$tenv" == "4" ]
		  then
			cmake_env="Visual Studio 10"
			vcpp=1
		else
			echo "This script does not support any other environment."
			exit 1
		fi
	else
		cmake_env="Unix Makefiles"
		linking="static"
	fi
	
	if [ "$vcpp" == "1" ]
	  then
		args="$args --enable-shared --disable-static"
	fi
			
	has_ffmpeg_binaries=0
	if [ "$use_cache" == "1" ]
	  then
		if [ "$vcpp" == "1" ]
		  then
			if test -d deps/ffmpeg-build/ &&
				test -f deps/ffmpeg-build/avcodec.dll &&
				test -f deps/ffmpeg-build/avdevice.dll &&
				test -f deps/ffmpeg-build/avformat.dll &&
				test -f deps/ffmpeg-build/avutil.dll &&
				test -f deps/ffmpeg-build/swscale.dll
			  then
				has_ffmpeg_binaries=1
			fi
		elif test -d deps/ffmpeg-build/ &&
			test -f deps/ffmpeg-build/libavcodec.a &&
			test -f deps/ffmpeg-build/libavdevice.a &&
			test -f deps/ffmpeg-build/libavformat.a &&
			test -f deps/ffmpeg-build/libavutil.a &&
			test -f deps/ffmpeg-build/libswscale.a
		  then
			has_ffmpeg_binaries=1
		fi
	fi
	
	if [ "$has_ffmpeg_binaries" == "1" ]
	  then
		echo "FFmpeg seems to be already built and ready to use. Do you want to skip the FFmpeg
compilation step? [Y/n]"
		read skip_ffmpeg
		
		if [ "$skip_ffmpeg" == "Y" ] ||
		   [ "$skip_ffmpeg" == "y" ] ||
		   [ "$skip_ffmpeg" == "" ]
		  then
			return;
		fi
	fi
	
	echo ""
	echo "==================== FFmpeg configuration and compilation ===================="
	echo ""
	
	free_decoders="theora flac vorbis"
	other_decoders=""
	full_decoders_list=""
	
	echo "
!!! IMPORTANT NOTICE !!!

I am going to let you choose the audio and video formats you will
be able to use with sfeMovie. First of all you should know that when
a patent covers an audio or video format, any decoder for this format
is also concerned. Thus if you decide to enable a decoder for a format,
you're responsible for the (possibly) bound patents and royalties
that may apply. See https://github.com/LaurentGomila/SFML/wiki/ProjectsfeMovie#license
for a little non-official sum up of the licenses and fees for the most common decoders.

FFmpeg provides decoders for the following formats:

$codec_list

Please now choose whether you want to enable
1 Free only (flac, vorbis, theora)
2 None but let me choose which to enable
3 All but let me choose which to disable
4 All

What is your choice? [1-4] (default is 1)"

	read enable_choice
	
	if [ "$enable_choice" == "" ] || [ "$enable_choice" == "1" ]
	  then
		full_decoders_list="flac vorbis theora"
	elif [ "$enable_choice" == "2" ]
	  then
	    echo "Option 2: choose the decoders you want to enable (separate names with a space):"
	    read full_decoders_list
	elif [ "$enable_choice" == "3" ]
	  then
	    echo "Option 3: choose the decoders you want to disable (separate names with a space):"
	    read to_disable
	    
	    for codec in $codec_list
	      do
	        disable=0
	        
	        for di_codec in $to_disable
	          do
	            if [ $di_codec == $codec ]
	              then
	                disable=1
	            fi
	        done
	        
	        if [ $disable == 0 ]
	          then
	            full_decoders_list="$full_decoders_list $codec"
	        fi
	    done
	elif [ "$enable_choice" == "4" ]
	  then
	    full_decoders_list="$codec_list"
	else
		echo "Invalid choice: $enable_choice"
	    exit 1
	fi
	
	echo ""
	echo "You have chosen to enable the following decoders:"
	echo "$full_decoders_list"
	echo ""
	
	echo "Do you confirm this choice? [Y/n]"
	read confirm_decoders
	
	if [ "$confirm_decoders" == "Y" ] ||
	   [ "$confirm_decoders" == "y" ] ||
	   [ "$confirm_decoders" == "" ]
	  then
	    # build ffmpeg
	    if test -d "deps/ffmpeg"
	      then
	        cd "deps/ffmpeg"
	        
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
	          	
	          	if [ "$has_vda" == "0" ]
	          	  then
	          	    os_flags="$os_flags --disable-vda"
	          	fi
	          		
	        	os_flags="$os_flags --cc=\"gcc -arch $macosx_arch\" --arch=$macosx_arch --target-os=darwin --enable-cross-compile --host-cflags=\"-arch $macosx_arch\" --host-ldflags=\"-arch $macosx_arch\""
	        fi
			
			if [ "$os" == "windows" ]
			  then
			    os_flags="--enable-memalign-hack --enable-w32threads"
			fi
			
			args="$args --disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-doc --disable-encoders --disable-decoders --disable-yasm $configure_flags $os_flags"
	        
			
			#setup VC++ env variables to find lib.exe
			if [ "$vcpp" == "1" ]
			  then
				old_path=`echo $PATH`
				export PATH="$PATH:/C/Program Files/Microsoft Visual Studio 9.0/Common7/IDE:/C/Program Files/Microsoft Visual Studio 9.0/VC/bin"
				export PATH="$PATH:/C/Program Files/Microsoft Visual Studio 10.0/Common7/IDE:/C/Program Files/Microsoft Visual Studio 10.0/VC/bin"
				export PATH="$PATH:/C/Program Files (x86)/Microsoft Visual Studio 9.0/Common7/IDE:/C/Program Files (x86)/Microsoft Visual Studio 9.0/VC/bin"
				export PATH="$PATH:/C/Program Files (x86)/Microsoft Visual Studio 10.0/Common7/IDE:/C/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin"
			fi
			
	        echo "./configure $args"
	        #sh $cmd
	        chmod u+x configure version.sh doc/texi2pod.pl
	        { echo "$args" | xargs ./configure; }
	        check_err
	        make clean
	        check_err
	        make --jobs=$jobsCount
	        check_err
	        
	        if [ "$vcpp" == "1" ]
	          then
				export PATH="$old_path"
			fi
	        
		    mkdir -p ../ffmpeg-build
			rm ../ffmpeg-build/*
		    
			echo "Copying libraries into ffmpeg-build"
			if [ "$vcpp" == "1" ]
			  then
				cp -v `find . -name "*.lib"` ../ffmpeg-build
				check_err
				cp -v `find . -name "*.dll"` ../ffmpeg-build
				check_err
			else
				cp -v `find . -name "*.a"` ../ffmpeg-build
				check_err
			fi
			
	        cd ../..
	    else
	    	echo "Missing directory ffmpeg-sources. Aborting."
	    	exit 1
	    fi
	else
		exit 1
	fi
	
	echo "Built ffmpeg"
}

function build_sfemovie()
{
	# run cmake and make
	echo "==================== sfeMovie compilation ===================="
	echo ""
	
	if test -f CMakeCache.txt
	  then
	    rm CMakeCache.txt
	fi
	echo "Running CMake..."
		
	if [ "$macosx_sdk" != "" ]
	  then
		echo "cmake -G \"$cmake_env\" -DCMAKE_OSX_SYSROOT:STRING=\"$macosx_sdk\" -DHAS_VDA:STRING=\"$has_vda\" CMakeLists.txt"
		cmake -G "$cmake_env" -DCMAKE_OSX_SYSROOT:STRING="$macosx_sdk" -DHAS_VDA="$has_vda" CMakeLists.txt
	else
		echo "cmake -G \"$cmake_env\" CMakeLists.txt"
		cmake -G "$cmake_env" CMakeLists.txt
	fi
	check_err
	
	if [ "$vcpp" == "1" ]
	  then
	    echo ""
	    echo "The files required to build sfeMovie for Visual Studio have been created."
		echo "Now open sfeMovie.sln and build sfeMovie within Visual Studio."
		echo ""
		echo "This script is over."
		exit 0
	else
		echo "Running make..."
		make --jobs=$jobsCount
		check_err
		
		echo "Built sfeMovie"
		
		if [ "$os" != "macosx" ]
		  then
			if ! test -d product/lib
			  then
				mkdir -p product/lib
			fi
			if ! test -d product/include
			  then
				mkdir -p product/include
			fi
		fi
		
		if [ "$os" == "macosx" ]
		  then
		  	ditto -v deps/SFML/extlibs/libs-osx/Frameworks/sndfile.framework product/sndfile.framework
			ditto -v deps/SFML/lib/ product/
			ditto -v sfeMovie.framework product/sfeMovie.framework
			
			if test -d deps/SFML/SFML.framework
			  then
			    ditto -v deps/SFML/SFML.framework product/SFML.framework
			fi
			
		elif [ "$os" == "windows" ]
		  then
			gccdir="deps/windows-binaries/gcc"
			sfdeps="deps/SFML/extlibs/bin/x86"
			cp -v ${sfdeps}/libsndfile-1.dll ${sfdeps}/openal32.dll ${gccdir}/libgcc_s_dw2-1.dll ${gccdir}/libstdc++-6.dll deps/SFML/lib/* product/lib
			cp -v libsfeMovie.dll libsfeMovie.dll.a product/lib
			cp -v include/* product/include
		fi
		
		echo "All of the required files have been copied to the \"product\" directory."
	fi
}

function main()
{
	# want help?
	if [ "$1" == "-h" ] ||
	   [ "$1" == "--help" ]
	   then
	     echo "Usage: $0 linux|windows|macosx [nocache]"
	else
		if [ "$1" != "linux" ] && [ "$1" != "windows" ] && [ "$1" != "macosx" ]
		  then
		    echo "Usage: $0 linux|windows|macosx [nocache]"
		else
			# do build process
			os="$1"
			
			# clean cached files
			if [ "$2" == "nocache" ]
			  then
			  	use_cache=1
			fi
			
			if [ "$use_cache" == "0" ]
			  then
				# remove CMake cache
				if test -f "CMakeCache.txt"
				  then
					rm "CMakeCache.txt"
					check_err
				fi
				
				# remove older compiled files
				if test -f "Makefile"
				  then
					make clean
					check_err
					
					rm "Makefile"
					check_err
				fi
			fi
			
			# build.. well it's written
			setup $*
			build_ffmpeg $*
			build_sfemovie $*
		fi
	fi
}

main $*

