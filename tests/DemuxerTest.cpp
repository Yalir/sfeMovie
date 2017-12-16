
#include <gtest/gtest.h>
#include <iostream>
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Utilities.hpp"
#include <SFML/Audio.hpp>

namespace
{
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
    
    DummyDelegate delegate;
}

TEST(Demuxer, AvailableCodecsTest)
{
	EXPECT_FALSE(sfe::Demuxer::getAvailableDemuxers().empty());
	EXPECT_FALSE(sfe::Demuxer::getAvailableDecoders().empty());
}

TEST(Demuxer, LoadingTest)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
    EXPECT_THROW(demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "non-existing-file.ogv", timer, delegate, delegate), std::runtime_error);
    EXPECT_NO_THROW(demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "small_1.ogv", timer, delegate, delegate));
	ASSERT_NE(demuxer, nullptr);
	
	const std::map<int, std::shared_ptr<sfe::Stream> >& streams = demuxer->getStreams();
	
	ASSERT_FALSE(streams.empty());
	
	unsigned videoStreamCount = 0;
	unsigned audioStreamCount = 0;
	
	// Check found streams
	for (auto it = streams.begin(); it != streams.end(); it++) {
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
	
	ASSERT_EQ(videoStreamCount, 1);
	ASSERT_EQ(audioStreamCount, 1);
	
	// Check stream feeding
	for (auto it = streams.begin(); it != streams.end(); it++) {
		EXPECT_TRUE(it->second->needsMoreData());
		EXPECT_FALSE(demuxer->didReachEndOfFile());
		demuxer->feedStream(*it->second);
		EXPECT_FALSE(demuxer->didReachEndOfFile());
		EXPECT_FALSE(it->second->needsMoreData());
	}
}

TEST(Demuxer, ShortOGVTest)
{
	std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	sf::Clock clock;
    demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "small_1.ogv", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> videoStream = *demuxer->getStreamsOfType(sfe::Video).begin();
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(videoStream->getStatus(), sfe::Stopped);
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
	timer->play();
	demuxer->update();
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(videoStream->getStatus(), sfe::Playing);
	EXPECT_EQ(audioStream->getStatus(), sfe::Playing);
	
	clock.restart();
	while (clock.getElapsedTime() < sf::seconds(8)) {
		demuxer->update();
		sf::sleep(sf::milliseconds(10));
	}
	
	EXPECT_TRUE(demuxer->didReachEndOfFile());
	EXPECT_EQ(videoStream->getStatus(), sfe::Stopped);
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
}

TEST(Demuxer, ShortWAVTest)
{
	std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
    demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "small_4.wav", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
	timer->play();
	demuxer->update();
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Playing);
	sf::sleep(sf::seconds(4));
	demuxer->update();
	EXPECT_TRUE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
}

TEST(Demuxer, LongWAVTest)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "long_1.wav", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
	timer->play();
	demuxer->update();
	EXPECT_FALSE(demuxer->didReachEndOfFile());
    EXPECT_EQ(audioStream->getStatus(), sfe::Playing);
	sf::sleep(sf::seconds(30));
	demuxer->update();
	EXPECT_TRUE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
}


TEST(Demuxer, ShortMP3Test)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	// With free codecs only, the demuxer is not supposed to be able to load MP3 medias
	
	EXPECT_THROW(demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "small_2.mp3", timer, delegate, delegate), std::runtime_error);
}

TEST(Demuxer, ShortFLACTest)
{
    std::shared_ptr<sfe::Demuxer> demuxer;
    std::shared_ptr<sfe::Timer> timer = std::make_shared<sfe::Timer>();
	demuxer = std::make_shared<sfe::Demuxer>(TEST_DATA_DIR "small_3.flac", timer, delegate, delegate);
	demuxer->selectFirstVideoStream();
	demuxer->selectFirstAudioStream();
	
	std::shared_ptr<sfe::Stream> audioStream = *demuxer->getStreamsOfType(sfe::Audio).begin();
	
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
	timer->play();
	demuxer->update();
	EXPECT_FALSE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Playing);
	sf::sleep(sf::seconds(5));
	demuxer->update();
	EXPECT_TRUE(demuxer->didReachEndOfFile());
	EXPECT_EQ(audioStream->getStatus(), sfe::Stopped);
}
