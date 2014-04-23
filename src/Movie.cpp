
/*
 *  Movie.cpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Lucas Soltic
 *  lucas.soltic@orange.fr
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <sfeMovie/Movie.hpp>
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Log.hpp"
#include <cmath>

namespace sfe {
	
	Movie::Movie(void) :
	m_demuxer(NULL),
	m_timer(NULL),
	m_sprite()
	{
	}
	
	Movie::~Movie(void)
	{
		cleanResources();
	}
	
	bool Movie::openFromFile(const std::string& filename)
	{
		cleanResources();
		
		try {
			m_timer = new Timer;
			m_demuxer = new Demuxer(filename, *m_timer, *this);
			
			std::set<Stream*> audioStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_AUDIO);
			std::set<Stream*> videoStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_VIDEO);
			
			if (audioStreams.size())
				m_demuxer->selectAudioStream(dynamic_cast<AudioStream*>(*audioStreams.begin()));
			
			if (videoStreams.size())
				m_demuxer->selectVideoStream(dynamic_cast<VideoStream*>(*videoStreams.begin()));
			
			if (!audioStreams.size() && !videoStreams.size()) {
				sfeLogError("No supported audio or video stream in this media");
				cleanResources();
				return false;
			} else {
				return true;
			}
		} catch (std::runtime_error& e) {
			sfeLogError(e.what());
			cleanResources();
			return false;
		}
	}
	
	void Movie::play(void)
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		m_timer->play();
		update();
	}
	
	void Movie::pause(void)
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		m_timer->pause();
		update();
	}
	
	void Movie::stop(void)
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		m_timer->stop();
		update();
	}
	
	void Movie::update(void)
	{
		CHECK(m_demuxer, "No media loaded");
		m_demuxer->update();
		
		if (getStatus() == Stopped && m_timer->getStatus() != Timer::Stopped) {
			m_timer->stop();
		}
		
		// Enable smoothing when the video is scaled
		sfe::VideoStream* vStream = m_demuxer->getSelectedVideoStream();
		if (vStream) {
			sf::Vector2f sc = getScale();
			
			if (std::fabs(sc.x - 1.f) < 0.00001 &&
				std::fabs(sc.y - 1.f) < 0.00001)
			{
				vStream->getVideoTexture().setSmooth(false);
			}
			else
			{
				vStream->getVideoTexture().setSmooth(true);
			}
		}
	}
	
	void Movie::setVolume(float volume)
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		std::set<Stream*> audioStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_AUDIO);
		std::set<Stream*>::const_iterator it;
		
		for (it = audioStreams.begin(); it != audioStreams.end(); it++) {
			AudioStream* audioStream = dynamic_cast<AudioStream*>(*it);
			audioStream->setVolume(volume);
		}
	}
	
	float Movie::getVolume(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
		
		CHECK(audioStream, "No selected audio stream, cannot return a volume");
		return audioStream->getVolume();
	}
	
	sf::Time Movie::getDuration(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		return m_demuxer->getDuration();
	}
	
	sf::Vector2i Movie::getSize(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
		
		if (videoStream) {
			return videoStream->getFrameSize();
		} else {
			sfeLogWarning("Movie::getSize() called but there is no active video stream");
			return sf::Vector2i(0, 0);
		}
	}
	
	void Movie::fitFrame(int x, int y, int width, int height, bool preserveRatio)
	{
		fitFrame(sf::IntRect(x, y, width, height), preserveRatio);
	}
	
	void Movie::fitFrame(sf::IntRect frame, bool preserveRatio)
	{
		sf::Vector2i movie_size = getSize();
		
		if (movie_size.x == 0 || movie_size.y == 0) {
			sfeLogWarning("Movie::fitFrame() called but the video frame size is (0, 0)");
			return;
		}
		
		sf::Vector2i wanted_size = sf::Vector2i(frame.width, frame.height);
		sf::Vector2i new_size;
		
		if (preserveRatio)
		{
			sf::Vector2i target_size = movie_size;
			
			float source_ratio = (float)movie_size.x / movie_size.y;
			float target_ratio = (float)wanted_size.x / wanted_size.y;
			
			if (source_ratio > target_ratio)
			{
				target_size.x = movie_size.x * ((float)wanted_size.x / movie_size.x);
				target_size.y = movie_size.y * ((float)wanted_size.x / movie_size.x);
			}
			else
			{
				target_size.x = movie_size.x * ((float)wanted_size.y / movie_size.y);
				target_size.y = movie_size.y * ((float)wanted_size.y / movie_size.y);
			}
			
			setScale((float)target_size.x / movie_size.x, (float)target_size.y / movie_size.y);
			new_size = target_size;
		}
		else
		{
			setScale((float)wanted_size.x / movie_size.x, (float)wanted_size.y / movie_size.y);
			new_size = wanted_size;
		}
		
		m_sprite.setPosition(frame.left + (wanted_size.x - new_size.x) / 2,
					frame.top + (wanted_size.y - new_size.y) / 2);
		setPosition(frame.left, frame.top);
	}
	
	float Movie::getFramerate(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
		CHECK(videoStream, "No selected video stream, cannot return a frame rate");
		return videoStream->getFrameRate();
	}
	
	unsigned int Movie::getSampleRate(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
		CHECK(audioStream, "No selected audio stream, cannot return a sample rate");
		return audioStream->getSampleRate();
	}
	
	unsigned int Movie::getChannelCount(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
		CHECK(audioStream, "No selected audio stream, cannot return a channel count");
		return audioStream->getChannelCount();
	}
	
	Movie::Status Movie::getStatus(void) const
	{
		Status st = Stopped;
		
		if (m_demuxer) {
			VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			Stream::Status vStatus = videoStream ? videoStream->getStatus() : Stream::Stopped;
			Stream::Status aStatus = audioStream ? audioStream->Stream::getStatus() : Stream::Stopped;
			
			if (vStatus == Stream::Playing || aStatus == Stream::Playing) {
				st = Movie::Playing;
			} else if (vStatus == Stream::Paused || aStatus == Stream::Paused) {
				st = Movie::Paused;
			}
		}
		
		return st;
	}
	
	sf::Time Movie::getPlayingOffset(void) const
	{
		CHECK(m_demuxer && m_timer, "No media loaded");
		return m_timer->getOffset();
	}
	
	const sf::Texture& Movie::getCurrentImage(void) const
	{
		static sf::Texture emptyTexture;
		
		if (m_sprite.getTexture()) {
			return *m_sprite.getTexture();
		} else {
			return emptyTexture;
		}
	}
	
	
	void Movie::cleanResources(void)
	{
		if (m_demuxer)
			delete m_demuxer, m_demuxer = NULL;
		
		if (m_timer)
			delete m_timer, m_timer = NULL;
	}
	
	void Movie::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		states.transform *= getTransform();
		target.draw(m_sprite, states);
	}
	
	void Movie::didUpdateImage(const VideoStream& sender, const sf::Texture& image)
	{
		if (m_sprite.getTexture() != &image) {
			m_sprite.setTexture(image);
		}
	}
	
} // namespace sfe
