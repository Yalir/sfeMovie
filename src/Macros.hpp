
#include <cstring>
#include <string>
#include <stdexcept>
#include <SFML/Config.hpp>

extern "C" {
	#include <libavutil/error.h>
}

#ifndef SFEMOVIE_MACROS_HPP
#define SFEMOVIE_MACROS_HPP

#define CHECK(value, message) if (!(value)) throw std::runtime_error(message);
#define CHECK0(value, message) CHECK(value == 0, message)
#define ONCE(sequence)\
{ static bool __done = false; if (!__done) { { sequence; } __done = true; } }

#define BENCH_START \
{ \
sf::Clock __bench;

#define BENCH_END(title) \
sfeLogDebug(std::string(title) + " took " + s(__bench.getElapsedTime().asMilliseconds()) + "ms"); \
}

#if defined(SFML_SYSTEM_WINDOWS)
	#ifdef av_err2str
	#undef av_err2str
	#endif

	namespace sfe {
		std::string ff_err2str(int code);
	}

	#define av_err2str sfe::ff_err2str
#endif

#ifndef LIBAVCODEC_VERSION
	typedef void *AVFormatContextRef;
	typedef void* AVCodecContextRef;
	typedef void* AVCodecRef;
	typedef void* AVPacketRef;
	typedef void* AVStreamRef;
	typedef void* AVFrameRef;
#else
	typedef AVFormatContext *AVFormatContextRef;
	typedef AVCodecContext* AVCodecContextRef;
	typedef AVCodec* AVCodecRef;
	typedef AVPacket* AVPacketRef;
	typedef AVStream* AVStreamRef;
	typedef AVFrame* AVFrameRef;
#endif

#endif
