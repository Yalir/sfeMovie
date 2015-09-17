
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE DemuxerTest
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Utilities.hpp"
#include <SFML/Audio.hpp>

class DummyDelegate : public sfe::VideoStream::Delegate, public sfe::SubtitleStream::Delegate {
	void didUpdateVideo(const sfe::VideoStream& sender, const sf::Texture& image)
	{
	}
    
    void didUpdateSubtitle(const sfe::SubtitleStream& sender,
                           const std::list<sf::Sprite>& subimages,
                           const std::list<sf::Vector2i>& positions)
    {
    }
    
    void didWipeOutSubtitles(const sfe::SubtitleStream& sender)
    {
    }
    
};

static DummyDelegate delegate;

BOOST_AUTO_TEST_CASE(DemuxerAvailableCodecsTest)
{
	BOOST_CHECK(!sfe::Demuxer::getAvailableDemuxers().empty());
	BOOST_CHECK(!sfe::Demuxer::getAvailableDecoders().empty());
}

BOOST_AUTO_TEST_CASE(DemuxerLoadingTest)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
    BOOST_CHECK_THROW(demuxer = std::make_shared<sfe::Demuxer>("non-existing-file.ogv", timer, delegate, delegate), std::runtime_error);
    BOOST_CHECK_NO_THROW(demuxer = std::make_shared<sfe::Demuxer>("small_1.ogv", timer, delegate, delegate));
	BOOST_REQUIRE(demuxer != NULL);
	
	const std::map<int, std::shared_ptr<sfe::Stream> >& streams = demuxer->getStreams();
	
	BOOST_CHECK(!streams.empty());
	
	unsigned videoStreamCount = 0;
	unsigned audioStreamCount = 0;
	
	std::map<int, std::shared_ptr<sfe::Stream> >::const_iterator it;
	
	// Check found streams
	for (it = streams.begin(); it != streams.end(); it++) {
		std::shared_ptr<sfe::Stream> stream = it->second;
		
		switch (stream->getStreamKind()) {
			case sfe::Video:
				videoStreamCount++;
				break;
			case sfe::Audio:
				audioStreamCount++;
				break;
			default:
				std::cout << "unknown stream kind" << std::endl;
		}
	}
	
	BOOST_CHECK(videoStreamCount == 1);
	BOOST_CHECK(audioStreamCount == 1);
	
	// Check stream feeding
	for (it = streams.begin(); it != streams.end(); it++) {
		BOOST_CHECK(it->second->needsMoreData());
		BOOST_CHECK(demuxer->didReachEndOfFile() == false);
		demuxer->feedStream(*it->second);
		BOOST_CHECK(demuxer->didReachEndOfFile() == false);
		BOOST_CHECK(it->second->needsMoreData() == false);
	}
}

BOOST_AUTO_TEST_CASE(DemuxerShortOGVTest)
{
	std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	sf::Clock clock;
    demuxer = std::make_shared<sfe::Demuxer>("small_1.ogv", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> videoStream = *demuxer->getStreamsOfType(sfe::Video).begin();
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(videoStream->getStatus() == sfe::Stopped);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer->play();
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(videoStream->getStatus() == sfe::Playing);
	BOOST_CHECK(audioStream->getStatus() == sfe::Playing);
	
	clock.restart();
	while (clock.getElapsedTime() < sf::seconds(8)) {
		demuxer->update();
		sf::sleep(sf::milliseconds(10));
	}
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
	BOOST_CHECK(videoStream->getStatus() == sfe::Stopped);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
}

BOOST_AUTO_TEST_CASE(DemuxerShortWAVTest)
{
	std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
    demuxer = std::make_shared<sfe::Demuxer>("small_4.wav", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer->play();
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Playing);
	sf::sleep(sf::seconds(4));
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
}

BOOST_AUTO_TEST_CASE(DemuxerLongWAVTest)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	demuxer = std::make_shared<sfe::Demuxer>("long_1.wav", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer->play();
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Playing);
	sf::sleep(sf::seconds(30));
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
}


BOOST_AUTO_TEST_CASE(DemuxerShortMP3Test)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	// With free codecs only, the demuxer is not supposed to be able to load MP3 medias
	
	BOOST_CHECK_THROW(demuxer = std::make_shared<sfe::Demuxer>("small_2.mp3", timer, delegate, delegate), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(DemuxerShortFLACTest)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	demuxer = std::make_shared<sfe::Demuxer>("small_3.flac", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer->play();
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Playing);
	sf::sleep(sf::seconds(5));
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
}
