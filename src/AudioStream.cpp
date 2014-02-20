
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "AudioStream.hpp"

namespace sfe {
	AudioStream::AudioStream(AVStreamRef stream, DataSource& dataSource) :
	Stream(stream, dataSource)
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
	
	AudioStream::Kind AudioStream::getStreamKind(void) const
	{
		return AUDIO_STREAM;
	}
	
	bool AudioStream::onGetData(sf::SoundStream::Chunk& data)
	{
		return false;
	}
	
	void AudioStream::onSeek(sf::Time timeOffset)
	{
		
	}
}
