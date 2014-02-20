
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "AudioStream.hpp"

namespace sfe {
	AudioStream::AudioStream(AVStreamRef stream) :
	Stream(stream)
	{
		
	}
	
	/** Default destructor
	 */
	AudioStream::~AudioStream(void)
	{
		
	}
	
	/* A/V control */
	
	/** Start playing this stream
	 */
	void AudioStream::play(void)
	{
		
	}
	
	/** Pause stream playback
	 */
	void AudioStream::pause(void)
	{
		
	}
	
	/** Stop stream playback and go back to beginning
	 */
	void AudioStream::stop(void)
	{
		
	}
}
