
/*
 *  MovieImpl.cpp
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

#include "MovieImpl.hpp"
#include "Demuxer.hpp"
#include "Timer.hpp"
#include "Log.hpp"
#include <cmath>
#include <iostream>

namespace sfe {
	MovieImpl::MovieImpl(sf::Transformable& movieView) :
	m_movieView(movieView),
	m_demuxer(NULL),
	m_timer(NULL),
	m_sprite()
	{
	}
	
	MovieImpl::~MovieImpl()
	{
		cleanResources();
	}
	
	bool MovieImpl::openFromFile(const std::string& filename)
	{
		cleanResources();
		
		try {
			m_timer = new Timer;
			m_demuxer = new Demuxer(filename, *m_timer, *this,*this);
			
			std::set<Stream*> audioStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_AUDIO);
			std::set<Stream*> videoStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_VIDEO);
			std::set<Stream*> subtitleStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_SUBTITLE);
			
			m_demuxer->selectFirstAudioStream();
			m_demuxer->selectFirstVideoStream();
			m_demuxer->selectFirstSubtitleStream();
			
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
	
	void MovieImpl::play()
	{
		if (m_demuxer && m_timer) {
			m_timer->play();
			update();
		} else {
			sfeLogError("Movie - No media loaded, cannot play");
		}
	}
	
	void MovieImpl::pause()
	{
		if (m_demuxer && m_timer) {
			m_timer->pause();
			update();
		} else {
			sfeLogError("Movie - No media loaded, cannot pause");
		}
	}
	
	void MovieImpl::stop()
	{
		if (m_demuxer && m_timer) {
			m_timer->stop();
			update();
		} else {
			sfeLogError("Movie - No media loaded, cannot stop");
		}
	}
	
	void MovieImpl::update()
	{
		if (m_demuxer && m_timer) {
			m_demuxer->update();
			
			if (getStatus() == Stopped && m_timer->getStatus() != Stopped) {
				m_timer->stop();
			}
			
			// Enable smoothing when the video is scaled
			sfe::SubtitleStream* sStream = m_demuxer->getSelectedSubtitleStream();
			sfe::VideoStream* vStream = m_demuxer->getSelectedVideoStream();
			if (vStream) {
				sf::Vector2f movieScale = m_movieView.getScale();
                sf::Vector2f subviewScale = m_sprite.getScale();
				
				if (std::fabs(movieScale.x - 1.f) < 0.00001 &&
					std::fabs(movieScale.y - 1.f) < 0.00001 &&
                    std::fabs(subviewScale.x - 1.f) < 0.00001 &&
                    std::fabs(subviewScale.y - 1.f) < 0.00001)
				{
					vStream->getVideoTexture().setSmooth(false);
				}
				else
				{
					vStream->getVideoTexture().setSmooth(true);
				}
			}
		} else {
			sfeLogWarning("Movie - No media loaded, nothing to update");
		}
	}
	
	void MovieImpl::setVolume(float volume)
	{
		if (m_demuxer && m_timer) {
			std::set<Stream*> audioStreams = m_demuxer->getStreamsOfType(MEDIA_TYPE_AUDIO);
			std::set<Stream*>::const_iterator it;
			
			for (it = audioStreams.begin(); it != audioStreams.end(); it++) {
				AudioStream* audioStream = dynamic_cast<AudioStream*>(*it);
				audioStream->setVolume(volume);
			}
		} else {
			sfeLogError("Movie - No media loaded, cannot set volume");
		}
	}
	
	float MovieImpl::getVolume() const
	{
		if (m_demuxer && m_timer) {
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			
			if (audioStream)
				return audioStream->getVolume();
		}
		
		sfeLogError("Movie - No selected audio stream, cannot return a volume");
		return 0;
	}
	
	sf::Time MovieImpl::getDuration() const
	{
		if (m_demuxer && m_timer) {
			return m_demuxer->getDuration();
		}
		
		sfeLogError("Movie - No media loaded, cannot return a duration");
		return sf::Time::Zero;
	}
	
	sf::Vector2i MovieImpl::getSize() const
	{
		if (m_demuxer && m_timer) {
			VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
			
			if (videoStream) {
				return videoStream->getFrameSize();
			}
		}
		sfeLogError("MovieImpl::getSize() called but there is no active video stream");
		return sf::Vector2i(0, 0);
	}
	
	void MovieImpl::fit(int x, int y, int width, int height, bool preserveRatio)
	{
		fit(sf::IntRect(x, y, width, height), preserveRatio);
	}
	
	void MovieImpl::fit(sf::IntRect frame, bool preserveRatio)
	{
		sf::Vector2i movie_size = getSize();
		
		if (movie_size.x == 0 || movie_size.y == 0) {
			sfeLogError("MovieImpl::fitFrame() called but the video frame size is (0, 0)");
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
			
			new_size = target_size;
		}
		else
		{
			new_size = wanted_size;
		}
		
		m_sprite.setPosition(frame.left + (wanted_size.x - new_size.x) / 2,
							 frame.top + (wanted_size.y - new_size.y) / 2);
		m_movieView.setPosition(frame.left, frame.top);
		m_sprite.setScale((float)new_size.x / movie_size.x, (float)new_size.y / movie_size.y);
	}
	
	float MovieImpl::getFramerate() const
	{
		if (m_demuxer && m_timer) {
			VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
			
			if (videoStream)
				return videoStream->getFrameRate();
		}
		
		sfeLogError("Movie - No selected video stream, cannot return a frame rate");
		return 0;
	}
	
	unsigned int MovieImpl::getSampleRate() const
	{
		if (m_demuxer && m_timer) {
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			
			if (audioStream)
				return audioStream->getSampleRate();
		}
		
		sfeLogError("Movie - No selected audio stream, cannot return a sample rate");
		return 0;
	}
	
	unsigned int MovieImpl::getChannelCount() const
	{
		if (m_demuxer && m_timer) {
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			if (audioStream)
				return audioStream->getChannelCount();
		}
		
		sfeLogError("Movie - No selected audio stream, cannot return a channel count");
		return 0;
	}
	
	Status MovieImpl::getStatus() const
	{
		Status st = Stopped;
		
		if (m_demuxer) {
			VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			Status vStatus = videoStream ? videoStream->getStatus() : Stopped;
			Status aStatus = audioStream ? audioStream->Stream::getStatus() : Stopped;
			
			if (vStatus == Playing || aStatus == Playing) {
				st = Playing;
			} else if (vStatus == Paused || aStatus == Paused) {
				st = Paused;
			}
		}
		
		return st;
	}
	
	sf::Time MovieImpl::getPlayingOffset() const
	{
		if (m_demuxer && m_timer) {
			return m_timer->getOffset();
		}
		
		sfeLogError("Movie - No media loaded, cannot return a playing offset");
		return sf::Time::Zero;
	}
	
	const sf::Texture& MovieImpl::getCurrentImage() const
	{
		static sf::Texture emptyTexture;
		
		if (m_sprite.getTexture()) {
			return *m_sprite.getTexture();
		} else {
			return emptyTexture;
		}
	}
	
	
	void MovieImpl::cleanResources()
	{
		if (m_demuxer)
			delete m_demuxer, m_demuxer = NULL;
		
		if (m_timer)
			delete m_timer, m_timer = NULL;
	}
	
	void MovieImpl::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		
		target.draw(m_sprite, states);
		for (uint32_t i = 0; i < m_subtitles.size(); ++i)
		{
			target.draw(m_subtitles[i], states);
		}
	}
	
	void MovieImpl::didUpdateVideo(const VideoStream& sender, const sf::Texture& image)
	{
		if (m_sprite.getTexture() != &image) {
			m_sprite.setTexture(image);
		}
	}

	void MovieImpl::didUpdateSubtitle(const SubtitleStream& sender, const std::vector<sf::Sprite>& subs)
	{
		m_subtitles = subs;
	}
}
