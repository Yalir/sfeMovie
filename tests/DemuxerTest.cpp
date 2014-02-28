
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE DemuxerTest
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Utilities.hpp"
#include <SFML/Audio.hpp>

BOOST_AUTO_TEST_CASE(DemuxerAvailableCodecsTest)
{
	const std::set<std::pair<std::string, sfe::MediaType> >& decoders = sfe::Demuxer::getAvailableDecoders();
	BOOST_CHECK(!decoders.empty());
	sfe::dumpAvailableDecoders();
	
//	std::set<std::pair<std::string, sfe::MediaType> >::const_iterator it;
//	for (it = decoders.begin(); it != decoders.end();it++) {
//		std::cout << "Decoder: " << it->first << " (" << sfe::MediaTypeToString(it->second) << ")" << std::endl;
//	}
}

BOOST_AUTO_TEST_CASE(DemuxerLoadingTest)
{
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	BOOST_CHECK_THROW(demuxer = new sfe::Demuxer("non-existing-file.ogv", timer), std::runtime_error);
	BOOST_CHECK_NO_THROW(demuxer = new sfe::Demuxer("small_1.ogv", timer));
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
	demuxer = new sfe::Demuxer("small_1.ogv", timer);
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	timer.play();
	sf::sleep(sf::seconds(7));
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
}

BOOST_AUTO_TEST_CASE(DemuxerShortWAVTest)
{
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	demuxer = new sfe::Demuxer("small_4.wav", timer);
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	timer.play();
	sf::sleep(sf::seconds(4));
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
}

BOOST_AUTO_TEST_CASE(DemuxerLongWAVTest)
{
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	demuxer = new sfe::Demuxer("long_1.wav", timer);
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	timer.play();
	sf::sleep(sf::seconds(30));
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
}


BOOST_AUTO_TEST_CASE(DemuxerShortMP3Test)
{
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	// With free codecs only, the demuxer is not supposed to be able to load MP3 medias
	BOOST_CHECK_THROW(demuxer = new sfe::Demuxer("small_2.mp3", timer), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(DemuxerShortFLACTest)
{
	sfe::Demuxer *demuxer = NULL;
	sfe::Timer timer;
	demuxer = new sfe::Demuxer("small_3.flac", timer);
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	timer.play();
	sf::sleep(sf::seconds(2));
	BOOST_CHECK(demuxer->didReachEndOfFile() == true);
}
