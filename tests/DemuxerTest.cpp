
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE DemuxerTest
#include <boost/test/unit_test.hpp>
#include <iostream>
#include "Demuxer.hpp"

BOOST_AUTO_TEST_CASE(DemuxerLoadingTest)
{
	sfe::Demuxer *demuxer = NULL;
	BOOST_CHECK_THROW(demuxer = new sfe::Demuxer("non-existing-file.ogv"), std::runtime_error);
	BOOST_CHECK_NO_THROW(demuxer = new sfe::Demuxer("small.ogv"));
	
	const std::map<int, sfe::Stream*>& streams = demuxer->getStreams();
	
	BOOST_CHECK(streams.size() > 0);
	
	unsigned videoStreamCount = 0;
	unsigned audioStreamCount = 0;
	
	std::map<int, sfe::Stream*>::const_iterator it;
	
	for (it = streams.begin(); it != streams.end(); it++) {
		sfe::Stream* stream = it->second;
		
		switch (stream->getStreamKind()) {
			case sfe::Stream::VIDEO_STREAM:
				videoStreamCount++;
				break;
			case sfe::Stream::AUDIO_STREAM:
				audioStreamCount++;
				break;
			default:
				std::cout << "unknown stream kind" << std::endl;
		}
	}
	
	BOOST_CHECK(videoStreamCount == 1);
	BOOST_CHECK(audioStreamCount == 1);
	
	for (it = streams.begin(); it != streams.end(); it++) {
		BOOST_CHECK(it->second->needsMoreData());
	}
	
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	demuxer->feedStreams();
	BOOST_CHECK(demuxer->didReachEndOfFile() == false);
	
	for (it = streams.begin(); it != streams.end(); it++) {
		BOOST_CHECK(it->second->needsMoreData() == false);
	}
	
}
