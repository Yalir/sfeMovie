#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import platform
import subprocess
import argparse

# Read arguments
parser = argparse.ArgumentParser(description='Run this script to launch the build process')
parser.add_argument('--sources', help='Full path to the project sources', required=True)
parser.add_argument('--config', choices={'Debug', 'Release'}, required = True)
parser.add_argument('--decoders', choices={'Free', 'All'}, default='Free')
parser.add_argument('--sfml_root', help='Full path to the SFML root directory')
parser.add_argument('--build', action = 'store_true', help = 'Whether the output should be built, otherwise only project generation step is done')
args = parser.parse_args()

# Setup environment
if args.sfml_root:
    os.environ['SFML_ROOT'] = args.sfml_root
if platform.system() == "Darwin":
    os.environ['PATH'] += ":/usr/local/bin"

# Display environment
print 'ENVIRON is: {}'.format(os.environ)
print 'Parameters are: {}'.format(args)
print 'Platform is: {}'.format(platform.system())
print 'Working directory is: {}'.format(os.getcwd())

# Check environment
if platform.system() != 'Windows':
    cmakePath = subprocess.check_output(["which", "cmake"])
    if not cmakePath:
        raise Exception('CMake not found')

# Make build directory
source_dir = os.path.abspath(args.sources)
build_dir = os.path.join(source_dir, "ci-output")
if not os.path.exists(build_dir):
    os.makedirs(build_dir)
os.chdir(build_dir)

# Select decoders
decoders = ''
if args.decoders == 'Free':
    decoders = 'adpcm_4xm;adpcm_adx;adpcm_afc;adpcm_ct;adpcm_dtk;adpcm_ea;adpcm_ea_maxis_xa;adpcm_ea_r1;adpcm_ea_r2;adpcm_ea_r3;adpcm_ea_xas;adpcm_g722;adpcm_g726;adpcm_g726le;adpcm_ima_amv;adpcm_ima_apc;adpcm_ima_dk3;adpcm_ima_dk4;adpcm_ima_ea_eacs;adpcm_ima_ea_sead;adpcm_ima_iss;adpcm_ima_oki;adpcm_ima_qt;adpcm_ima_rad;adpcm_ima_smjpeg;adpcm_ima_wav;adpcm_ima_ws;adpcm_ms;adpcm_sbpro_2;adpcm_sbpro_3;adpcm_sbpro_4;adpcm_swf;adpcm_thp;adpcm_thp_le;adpcm_vima;adpcm_xa;adpcm_yamaha;interplay_dpcm;pcm_alaw;pcm_bluray;pcm_dvd;pcm_f32be;pcm_f32le;pcm_f64be;pcm_f64le;pcm_lxf;pcm_mulaw;pcm_s16be;pcm_s16be_planar;pcm_s16le;pcm_s16le_planar;pcm_s24be;pcm_s24daud;pcm_s24le;pcm_s24le_planar;pcm_s32be;pcm_s32le;pcm_s32le_planar;pcm_s8;pcm_s8_planar;pcm_u16be;pcm_u16le;pcm_u24be;pcm_u24le;pcm_u32be;pcm_u32le;pcm_u8;pcm_zork;roq_dpcm;sol_dpcm;xan_dpcm;theora;flac;vorbis;vp8;vp9;opus'
else:
    decoders = 'aac;aac_fixed;aac_latm;aasc;ac3;ac3_fixed;adpcm_4xm;adpcm_adx;adpcm_afc;adpcm_ct;adpcm_dtk;adpcm_ea;adpcm_ea_maxis_xa;adpcm_ea_r1;adpcm_ea_r2;adpcm_ea_r3;adpcm_ea_xas;adpcm_g722;adpcm_g726;adpcm_g726le;adpcm_ima_amv;adpcm_ima_apc;adpcm_ima_dk3;adpcm_ima_dk4;adpcm_ima_ea_eacs;adpcm_ima_ea_sead;adpcm_ima_iss;adpcm_ima_oki;adpcm_ima_qt;adpcm_ima_rad;adpcm_ima_smjpeg;adpcm_ima_wav;adpcm_ima_ws;adpcm_ms;adpcm_sbpro_2;adpcm_sbpro_3;adpcm_sbpro_4;adpcm_swf;adpcm_thp;adpcm_thp_le;adpcm_vima;adpcm_xa;adpcm_yamaha;aic;alac;alias_pix;als;amrnb;amrwb;amv;anm;ansi;ape;apng;ass;asv1;asv2;atrac1;atrac3;atrac3p;aura;aura2;avrn;avrp;avs;avui;ayuv;bethsoftvid;bfi;bink;binkaudio_dct;binkaudio_rdft;bintext;bmp;bmv_audio;bmv_video;brender_pix;c93;cavs;ccaption;cdgraphics;cdxl;cinepak;cljr;cllc;comfortnoise;cook;cpia;cscd;cyuv;dca;dds;dfa;dirac;dnxhd;dpx;dsd_lsbf;dsd_lsbf_planar;dsd_msbf;dsd_msbf_planar;dsicinaudio;dsicinvideo;dss_sp;dvbsub;dvdsub;dvvideo;dxa;dxtory;eac3;eacmv;eamad;eatgq;eatgv;eatqi;eightbps;eightsvx_exp;eightsvx_fib;escape124;escape130;evrc;exr;ffv1;ffvhuff;ffwavesynth;fic;flac;flashsv;flashsv2;flic;flv;fourxm;fraps;frwu;g2m;g723_1;g729;gif;gsm;gsm_ms;h261;h263;h263i;h263p;h264;h264_crystalhd;h264_mmal;h264_qsv;h264_vda;h264_vdpau;hap;hevc;hevc_qsv;hnm4_video;hq_hqa;hqx;huffyuv;iac;idcin;idf;iff_byterun1;iff_ilbm;imc;indeo2;indeo3;indeo4;indeo5;interplay_dpcm;interplay_video;jacosub;jpeg2000;jpegls;jv;kgv1;kmvc;lagarith;libcelt;libdcadec;libfdk_aac;libgsm;libgsm_ms;libilbc;libopencore_amrnb;libopencore_amrwb;libopenjpeg;libopus;libschroedinger;libspeex;libstagefright_h264;libutvideo;libvorbis;libvpx_vp8;libvpx_vp9;libzvbi_teletext;loco;mace3;mace6;mdec;metasound;microdvd;mimic;mjpeg;mjpegb;mlp;mmvideo;motionpixels;movtext;mp1;mp1float;mp2;mp2float;mp3;mp3adu;mp3adufloat;mp3float;mp3on4;mp3on4float;mpc7;mpc8;mpeg1_vdpau;mpeg1video;mpeg2_crystalhd;mpeg2_qsv;mpeg2video;mpeg4;mpeg4_crystalhd;mpeg4_vdpau;mpeg_vdpau;mpeg_xvmc;mpegvideo;mpl2;msa1;msmpeg4_crystalhd;msmpeg4v1;msmpeg4v2;msmpeg4v3;msrle;mss1;mss2;msvideo1;mszh;mts2;mvc1;mvc2;mxpeg;nellymoser;nuv;on2avc;opus;paf_audio;paf_video;pam;pbm;pcm_alaw;pcm_bluray;pcm_dvd;pcm_f32be;pcm_f32le;pcm_f64be;pcm_f64le;pcm_lxf;pcm_mulaw;pcm_s16be;pcm_s16be_planar;pcm_s16le;pcm_s16le_planar;pcm_s24be;pcm_s24daud;pcm_s24le;pcm_s24le_planar;pcm_s32be;pcm_s32le;pcm_s32le_planar;pcm_s8;pcm_s8_planar;pcm_u16be;pcm_u16le;pcm_u24be;pcm_u24le;pcm_u32be;pcm_u32le;pcm_u8;pcm_zork;pcx;pgm;pgmyuv;pgssub;pictor;pjs;png;ppm;prores;prores_lgpl;ptx;qcelp;qdm2;qdraw;qpeg;qtrle;r10k;r210;ra_144;ra_288;ralf;rawvideo;realtext;rl2;roq;roq_dpcm;rpza;rv10;rv20;rv30;rv40;s302m;sami;sanm;sgi;sgirle;shorten;sipr;smackaud;smacker;smc;smvjpeg;snow;sol_dpcm;sonic;sp5x;srt;ssa;stl;subrip;subviewer;subviewer1;sunrast;svq1;svq3;tak;targa;targa_y216;tdsc;text;theora;thp;tiertexseqvideo;tiff;tmv;truehd;truemotion1;truemotion2;truespeech;tscc;tscc2;tta;twinvq;txd;ulti;utvideo;v210;v210x;v308;v408;v410;vb;vble;vc1;vc1_crystalhd;vc1_qsv;vc1_vdpau;vc1image;vcr1;vima;vmdaudio;vmdvideo;vmnc;vorbis;vp3;vp5;vp6;vp6a;vp6f;vp7;vp8;vp9;vplayer;vqa;wavpack;webp;webvtt;wmalossless;wmapro;wmav1;wmav2;wmavoice;wmv1;wmv2;wmv3;wmv3_crystalhd;wmv3_vdpau;wmv3image;wnv1;ws_snd1;xan_dpcm;xan_wc3;xan_wc4;xbin;xbm;xface;xl;xsub;xwd;y41p;yop;yuv4;zero12v;zerocodec;zlib;zmbv'

# Define generator
generatorArg = None
if platform.system() == 'Windows':
    generatorArg = ['-G', 'Visual Studio 15']
elif platform.system() == "Darwin":
    generatorArg = ['-G', 'Xcode']

# Configure
command = ["cmake", source_dir, "-DSFEMOVIE_ENABLED_DECODERS=" + decoders]

if generatorArg:
	command.extend(generatorArg)
if platform.system() == 'Linux':
    command.append('-DCMAKE_BUILD_TYPE=' + args.config)

print 'Execute: {}'.format(command)
subprocess.check_call(command)

if args.build:
    # Build
    command = ["cmake", "--build", ".", "--config", args.config]
    print 'Execute: {}'.format(command)
    subprocess.check_call(command)

    # Create distributable archive
    command = ["cmake", "--build", ".", "--config", args.config, "--target", "package"]
    print 'Execute: {}'.format(command)
    subprocess.check_call(command)
