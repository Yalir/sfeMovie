
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
		CHECK(videoStream, "No selected video stream, cannot return a frame size");
		return videoStream->getFrameSize();
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
	
	void Movie::cleanResources(void)
	{
		if (m_demuxer)
			delete m_demuxer, m_demuxer = NULL;
		
		if (m_timer)
			delete m_timer, m_timer = NULL;
	}
	
	void Movie::draw(sf::RenderTarget& Target, sf::RenderStates states) const
	{
		states.transform *= getTransform();
		Target.draw(m_sprite, states);
	}
	
	void Movie::didUpdateImage(const VideoStream& sender, const sf::Texture& image)
	{
		if (m_sprite.getTexture() == NULL) {
			m_sprite.setTexture(image);
		}
	}
	
} // namespace sfe
