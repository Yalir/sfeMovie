#!/bin/sh

os=""
macosx_arch="i386" # ppc or i386 or x86_64
use_cache=1
codec_list="aac aasc ac3 adpcm_4xm adpcm_adx adpcm_ct adpcm_ea adpcm_ea_maxis_xa adpcm_ea_r1 adpcm_ea_r2 adpcm_ea_r3 adpcm_ea_xas adpcm_g726 adpcm_ima_amv adpcm_ima_dk3 adpcm_ima_dk4 adpcm_ima_ea_eacs adpcm_ima_ea_sead adpcm_ima_iss adpcm_ima_qt adpcm_ima_smjpeg adpcm_ima_wav adpcm_ima_ws adpcm_ms adpcm_sbpro_2 adpcm_sbpro_3 adpcm_sbpro_4 adpcm_swf adpcm_thp adpcm_xa adpcm_yamaha alac als amrnb amv anm ape asv1 asv2 atrac1 atrac3 aura aura2 avs bethsoftvid bfi bink binkaudio_dct binkaudio_rdft bmp c93 cavs cdgraphics cinepak cljr cook cscd cyuv dca dnxhd dpx dsicinaudio dsicinvideo dvbsub dvdsub dvvideo dxa eac3 eacmv eamad eatgq eatgv eatqi eightbps eightsvx_exp eightsvx_fib escape124 ffv1 ffvhuff flac flashsv flic flv fourxm fraps frwu gif h261 h263 h263i h264 h264_vdpau huffyuv idcin iff_byterun1 iff_ilbm imc indeo2 indeo3 indeo5 interplay_dpcm interplay_video jpegls kgv1 kmvc libdirac libfaad libgsm libgsm_ms libopencore_amrnb libopencore_amrwb libopenjpeg libschroedinger libspeex libvpx loco mace3 mace6 mdec mimic mjpeg mjpegb mlp mmvideo motionpixels mp1 mp2 mp3 mp3adu mp3on4 mpc7 mpc8 mpeg1_vdpau mpeg1video mpeg2video mpeg4 mpeg4_vdpau mpeg_vdpau mpeg_xvmc mpegvideo msmpeg4v1 msmpeg4v2 msmpeg4v3 msrle msvideo1 mszh nellymoser nuv pam pbm pcm_alaw pcm_bluray pcm_dvd pcm_f32be pcm_f32le pcm_f64be pcm_f64le pcm_mulaw pcm_s16be pcm_s16le pcm_s16le_planar pcm_s24be pcm_s24daud pcm_s24le pcm_s32be pcm_s32le pcm_s8 pcm_u16be pcm_u16le pcm_u24be pcm_u24le pcm_u32be pcm_u32le pcm_u8 pcm_zork pcx pgm pgmyuv pgssub png ppm ptx qcelp qdm2 qdraw qpeg qtrle r210 ra_144 ra_288 rawvideo rl2 roq roq_dpcm rpza rv10 rv20 rv30 rv40 sgi shorten sipr smackaud smacker smc snow sol_dpcm sonic sp5x sunrast svq1 svq3 targa theora thp tiertexseqvideo tiff tmv truehd truemotion1 truemotion2 truespeech tscc tta twinvq txd ulti v210 v210x vb vc1 vc1_vdpau vcr1 vmdaudio vmdvideo vmnc vorbis vp3 vp5 vp6 vp6a vp6f vqa wavpack wmapro wmav1 wmav2 wmavoice wmv1 wmv2 wmv3 wmv3_vdpau wnv1 ws_snd1 xan_dpcm xan_wc3 xl xsub yop zlib zmbv"

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
	if [ "$use_cache" == "1" ]
	  then
		if test -d deps/ffmpeg-build/ &&
			test -f deps/ffmpeg-build/libavcodec.a &&
			test -f deps/ffmpeg-build/libavdevice.a &&
			test -f deps/ffmpeg-build/libavformat.a &&
			test -f deps/ffmpeg-build/libavutil.a &&
			test -f deps/ffmpeg-build/libswscale.a
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
	    if test -d "ffmpeg-sources"
	      then
	        cd "ffmpeg-sources"
	        
	        configure_flags="";
	        
	        for codec in $full_decoders_list
	          do
	            configure_flags="$configure_flags --enable-decoder=$codec"
	        done
	        
	        if [ "$os" == "macosx" ]
	          then
	        	os_flags="--sysroot=/Developer/SDKs/MacOSX10.5.sdk --cc=\"gcc -arch $macosx_arch\" --arch=$macosx_arch --target-os=darwin --enable-cross-compile --host-cflags=\"-arch $macosx_arch\" --host-ldflags=\"-arch $macosx_arch\""
	        fi
			
			if [ "$os" == "windows" ]
			  then
			    os_flags="--enable-memalign-hack"
			fi
			
			args="--disable-ffmpeg --disable-ffplay --disable-ffprobe --disable-ffserver --disable-encoders --disable-decoders $configure_flags $os_flags"
	        
	        echo "./configure $args"
	        #sh $cmd
	        chmod u+x configure version.sh doc/texi2pod.pl
	        { echo "$args" | xargs ./configure; } && make clean && make
	        
	        #check_err
	        #make
	        
	        check_err
	        
	        if ! test -d ../deps/ffmpeg-build
	          then
		        mkdir ../deps/ffmpeg-build
		    fi
		    
	        cp -v `find . -name "*.a"` ../deps/ffmpeg-build
	        cd ..
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
	
	tenv=""
	if [ "$os" == "windows" ]
	  then
		echo "Choose your target environment (default is 1):"
		echo "1. GCC"
		echo "2. Visual Studio 2005"
		echo "3. Visual Studio 2008"
		echo "4. Visual Studio 2010"
		echo "5. Other"
		echo ""
		
		read tenv
	fi
	
	cmake_env=""
	if [ "$tenv" == "" ] || [ "$tenv" == "1" ]
	  then
	    cmake_env="Unix Makefiles"
	elif [ "$tenv" == "2" ]
	  then
	    cmake_env="Visual Studio 8 2005"
	elif [ "$tenv" == "3" ]
	  then
	    cmake_env="Visual Studio 9 2008"
	elif [ "$tenv" == "4" ]
	  then
	    cmake_env="Visual Studio 10"
	else
	    echo "This script does not support any other environment."
		echo "Use CMake with the righ generator."
		exit 1
	fi
	
	echo "Running CMake..."
	echo "cmake -G \"$cmake_env\" CMakeLists.txt"
	cmake -G "$cmake_env" CMakeLists.txt
	check_err
	
	if [ "$cmake_env" != "Unix Makefiles" ]
	  then
	    echo ""
	    echo "The files required to build sfeMovie for Visual Studio have been created."
		echo "Now run Visual Studio, open sfeMovie.sln and follow these instructions:
- go to the sfe-movie properties panel. There, go to Linker > Input and add the following lines:
libavdevice.a
libavformat.a
libavcodec.a
libavutil.a
libswscale.a
libz.a
libgcc.a
libmingwex.a
libmoldname.a
- build the solution"
		echo ""
		echo "This script is over."
		exit 0
	else
		echo "Running make..."
		make
		check_err
		
		echo "Built sfeMovie"
		
		if ! test -d product/lib
		  then
			mkdir -p product/lib
		fi
		
		if ! test -d product/include
		  then
			mkdir -p product/include
		fi
		
		if [ "$os" == "macosx" ]
		  then
			cp -v -R deps/macosx-binaries/* product/lib
			cp -v libsfe-movie.dylib product/lib
		elif [ "$os" == "windows" ]
		  then
			cp -v -R deps/windows-binaries/* product/lib
			rm -v product/lib/libz.a
			cp -v libsfe-movie.dll libsfe-movie.dll.a product/lib
		fi
		
		cp -v include/* product/include
		
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
			build_ffmpeg $*
			build_sfemovie $*
		fi
	fi
}

main $*

