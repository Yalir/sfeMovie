
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE DemuxerTest
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Utilities.hpp"
#include <SFML/Audio.hpp>

class DummyDelegate : public sfe::VideoStreamDelegate {
	void didUpdateImage(const sfe::VideoStream& sender, const sf::Texture& image)
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
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	BOOST_CHECK_THROW(demuxer = new sfe::Demuxer("non-existing-file.ogv", timer, delegate), std::runtime_error);
	BOOST_CHECK_NO_THROW(demuxer = new sfe::Demuxer("small_1.ogv", timer, delegate));
	BOOST_REQUIRE(demuxer != NULL);
	
	const std::map<int, sfe::Stream*>& streams = demuxer->getStreams();
	
	BOOST_CHECK(streams.size() > 0);
	
	unsigned videoStreamCount = 0;
	unsigned audioStreamCount = 0;
	
	std::map<int, sfe::Stream*>::const_iterator it;
	
	// Check found streams
	for (it = streams.begin(); it != streams.end(); it++) {
		sfe::Stream* stream = it->second;
		
		switch (stream->getStreamKind()) {
			case sfe::MEDIA_TYPE_VIDEO:
				videoStreamCount++;
				break;
			case sfe::MEDIA_TYPE_AUDIO:
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
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	sf::Clock clock;
	demuxer = new sfe::Demuxer("small_1.ogv", timer, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	sfe::Stream* videoStream = *demuxer->getStreamsOfType(sfe::MEDIA_TYPE_VIDEO).begin();
	sfe::Stream* audioStream = *demuxer->getStreamsOfType(sfe::MEDIA_TYPE_AUDIO).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(videoStream->getStatus() == sfe::Stopped);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer.play();
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
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	demuxer = new sfe::Demuxer("small_4.wav", timer, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	sfe::Stream* audioStream = *demuxer->getStreamsOfType(sfe::MEDIA_TYPE_AUDIO).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer.play();
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
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	demuxer = new sfe::Demuxer("long_1.wav", timer, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	sfe::Stream* audioStream = *demuxer->getStreamsOfType(sfe::MEDIA_TYPE_AUDIO).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer.play();
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
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	// With free codecs only, the demuxer is not supposed to be able to load MP3 medias
	
	BOOST_CHECK_THROW(demuxer = new sfe::Demuxer("small_2.mp3", timer, delegate), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(DemuxerShortFLACTest)
{
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	demuxer = new sfe::Demuxer("small_3.flac", timer, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	sfe::Stream* audioStream = *demuxer->getStreamsOfType(sfe::MEDIA_TYPE_AUDIO).begin();
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
	timer.play();
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	BOOST_CHECK(audioStream->getStatus() == sfe::Playing);
	sf::sleep(sf::seconds(5));
	demuxer->update();
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
	BOOST_CHECK(audioStream->getStatus() == sfe::Stopped);
}
