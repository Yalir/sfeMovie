
/** Define portable import / export macros
 */
#if defined(SFML_SYSTEM_WINDOWS) && defined(_MSC_VER)
	#ifdef SFE_EXPORTS
		/** From DLL side, we must export
		 */
		#define SFE_API __declspec(dllexport)
	#else
		/** From client application side, we must import
		 */
		#define SFE_API __declspec(dllimport)
	#endif

	/** For Visual C++ compilers, we also need to turn off this annoying C4251 warning.
	 * You can read lots ot different things about it, but the point is the code will
	 * just work fine, and so the simplest way to get rid of this warning is to disable it
	 */
	#ifdef _MSC_VER
		#pragma warning(disable : 4251)
	#endif
#else
	#define SFE_API
#endif

#define CHECK(value, message) if (!(value)) throw std::runtime_error(message);
#define CHECK0(value, message) CHECK(value == 0, message)
#define ONCE(sequence)\
{ static bool __done = false; if (!__done) { { sequence; } __done = true; } }

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
