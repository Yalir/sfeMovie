
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include "Demuxer.hpp"
#include "Utilities.hpp"
#include "Log.hpp"
#include "AudioStream.hpp"
#include "VideoStream.hpp"
#include <iostream>

void my_pause()
{
#ifdef SFML_SYSTEM_WINDOWS
	system("PAUSE");
#endif
}

class DummyDelegate : public sfe::VideoStream::Delegate {
	void didUpdateImage(const sfe::VideoStream& sender, const sf::Texture& image)
	{
		
	}
};

bool finished(const sfe::Demuxer& demuxer)
{
	std::set<sfe::Stream*> audioStreams = demuxer.getStreamsOfType(sfe::MEDIA_TYPE_AUDIO);
	std::set<sfe::Stream*>::iterator it;
	
	for (it = audioStreams.begin(); it != audioStreams.end(); it++) {
		sfe::AudioStream* audioStream = dynamic_cast<sfe::AudioStream*>(*it);
		
		if (audioStream->Stream::getStatus() == sfe::Stream::Playing) {
			return false;
		}
	}
	
	return true;
}

int main(int argc, const char *argv[])
{
	// Some settings
	
	if (argc < 2)
	{
		std::cout << "Usage: " << std::string(argv[0]) << " media_path" << std::endl;
		my_pause();
		return 1;
	}
	
	std::string mediaFile = std::string(argv[1]);
	std::cout << "Going to open movie file \"" << mediaFile << "\"" << std::endl;
	
	sfe::dumpAvailableDecoders();
	sfe::Log::setMask(sfe::Log::DebugMask | sfe::Log::WarningMask);
	
	sfe::Timer timer;
	DummyDelegate delegate;
	sfe::Demuxer demuxer(mediaFile, timer, delegate);
	demuxer.selectFirstVideoStream();
	demuxer.selectFirstAudioStream();
	
	timer.play();
	demuxer.update();
	
	while (!finished(demuxer)) {
		demuxer.update();
		sf::sleep(sf::milliseconds(100));
	}
	
	return 0;
}

