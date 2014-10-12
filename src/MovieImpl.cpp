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
#include "Utilities.hpp"
#include <cmath>
#include <iostream>

namespace sfe {
	MovieImpl::MovieImpl(sf::Transformable& movieView) :
		m_movieView(movieView),
		m_demuxer(NULL),
		m_timer(NULL),
		m_videoSprite(),
		m_scaleX(1.0f),
		m_scaleY(1.0f)
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
			m_audioStreamsDesc = m_demuxer->computeStreamDescriptors(Audio);
			m_videoStreamsDesc = m_demuxer->computeStreamDescriptors(Video);
			m_subtitleStreamsDesc = m_demuxer->computeStreamDescriptors(Subtitle);

			std::set<Stream*> audioStreams = m_demuxer->getStreamsOfType(Audio);
			std::set<Stream*> videoStreams = m_demuxer->getStreamsOfType(Video);
			std::set<Stream*> subtitleStreams = m_demuxer->getStreamsOfType(Subtitle);

			m_demuxer->selectFirstAudioStream();
			m_demuxer->selectFirstVideoStream();
			m_demuxer->selectFirstSubtitleStream();

			if (!audioStreams.size() && !videoStreams.size()) {
				sfeLogError("Movie::openFromFile() - No supported audio or video stream in this media");
				cleanResources();
				return false;
			}
			else {
                if (videoStreams.size()) {
                    sf::Vector2i size = getSize();
                    m_displayFrame = sf::IntRect(0, 0, size.x, size.y);
                }
                
				return true;
			}
		}
		catch (std::runtime_error& e) {
			sfeLogError(e.what());
			cleanResources();
			return false;
		}
	}

	const Streams& MovieImpl::getStreams(MediaType type) const
	{
		switch (type) {
		case Audio: return m_audioStreamsDesc;
		case Video: return m_videoStreamsDesc;
		case Subtitle: return m_subtitleStreamsDesc;
		default: CHECK(false, "Movie::getStreams() - Unknown stream type:" + MediaTypeToString(type));
		}
	}

	void MovieImpl::selectStream(const StreamDescriptor& streamDescriptor)
	{
		if (!m_demuxer || !m_timer) {
			sfeLogError("Movie::selectStream() - cannot select a stream with no opened media");
			return;
		}

		if (m_timer->getStatus() != Stopped) {
			sfeLogError("Movie::selectStream() - cannot select a stream while media is not stopped");
			return;
		}

		std::map<int, Stream*> streams = m_demuxer->getStreams();
		std::map<int, Stream*>::iterator it = streams.find(streamDescriptor.identifier);
		Stream* streamToSelect = NULL;

		if (it != streams.end()) {
			streamToSelect = it->second;
		}

		switch (streamDescriptor.type) {
		case Audio:
			m_demuxer->selectAudioStream(dynamic_cast<AudioStream*>(streamToSelect));
			break;
		case Video:
			m_demuxer->selectVideoStream(dynamic_cast<VideoStream*>(streamToSelect));
			break;
		case Subtitle:
			m_demuxer->selectSubtitleStream(dynamic_cast<SubtitleStream*>(streamToSelect));
			break;
		default:
			sfeLogWarning("Movie::selectStream() - stream activation for stream of kind "
				+ MediaTypeToString(it->second->getStreamKind()) + " is not supported");
			break;
		}
	}

	void MovieImpl::play()
	{
		if (m_demuxer && m_timer) {
			if (m_timer->getStatus() == Playing) {
				sfeLogError("Movie::play() - media is already playing");
				return;
			}

			m_timer->play();
			update();
		}
		else {
			sfeLogError("Movie::play() - No media loaded, cannot play");
		}
	}

	void MovieImpl::pause()
	{
		if (m_demuxer && m_timer) {
			if (m_timer->getStatus() == Paused) {
				sfeLogError("Movie::pause() - media is already paused");
				return;
			}

			m_timer->pause();
			update();
		}
		else {
			sfeLogError("Movie::pause() - No media loaded, cannot pause");
		}
	}

	void MovieImpl::stop()
	{
		if (m_demuxer && m_timer) {
			if (m_timer->getStatus() == Stopped) {
				sfeLogError("Movie::stop() - media is already stopped");
				return;
			}

			m_timer->stop();
			update();
		}
		else {
			sfeLogError("Movie::stop() - No media loaded, cannot stop");
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
			sfe::VideoStream* vStream = m_demuxer->getSelectedVideoStream();
			if (vStream) {
				sf::Vector2f movieScale = m_movieView.getScale();
				sf::Vector2f subviewScale = m_videoSprite.getScale();

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
		}
		else {
			sfeLogWarning("Movie::update() - No media loaded, nothing to update");
		}
	}

	void MovieImpl::setVolume(float volume)
	{
		if (m_demuxer && m_timer) {
			std::set<Stream*> audioStreams = m_demuxer->getStreamsOfType(Audio);
			std::set<Stream*>::const_iterator it;

			for (it = audioStreams.begin(); it != audioStreams.end(); it++) {
				AudioStream* audioStream = dynamic_cast<AudioStream*>(*it);
				audioStream->setVolume(volume);
			}
		}
		else {
			sfeLogError("Movie::setVolume() - No media loaded, cannot set volume");
		}
	}

	float MovieImpl::getVolume() const
	{
		if (m_demuxer && m_timer) {
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();

			if (audioStream)
				return audioStream->getVolume();
		}

		sfeLogError("Movie::getVolume() - No selected audio stream or no media loaded, cannot return a volume");
		return 0;
	}

	sf::Time MovieImpl::getDuration() const
	{
		if (m_demuxer && m_timer) {
			return m_demuxer->getDuration();
		}

		sfeLogError("Movie::getDuration() - No media loaded, cannot return a duration");
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
		sfeLogError("Movie::getSize() called but there is no active video stream");
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
			sfeLogError("Movie::fit() called but the video frame size is (0, 0)");
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

		m_videoSprite.setPosition(frame.left + (wanted_size.x - new_size.x) / 2,
			frame.top + (wanted_size.y - new_size.y) / 2);
		m_movieView.setPosition(frame.left, frame.top);
		m_scaleX = (float)new_size.x / movie_size.x;
		m_scaleY = (float)new_size.y / movie_size.y;
		m_videoSprite.setScale(m_scaleX, m_scaleY);
        m_displayFrame = frame;

		for (uint32_t i = 0; i < m_subtitleSprites.size(); ++i)
		{
			m_subtitleSprites[i].setScale(m_scaleX, m_scaleY);
		}
	}

	float MovieImpl::getFramerate() const
	{
		if (m_demuxer && m_timer) {
			VideoStream* videoStream = m_demuxer->getSelectedVideoStream();

			if (videoStream)
				return videoStream->getFrameRate();
		}

		sfeLogError("Movie::getFramerate() - No selected video stream or no media loaded, cannot return a frame rate");
		return 0;
	}

	unsigned int MovieImpl::getSampleRate() const
	{
		if (m_demuxer && m_timer) {
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();

			if (audioStream)
				return audioStream->getSampleRate();
		}

		sfeLogError("Movie::getSampleRate - No selected audio stream or no media loaded, cannot return a sample rate");
		return 0;
	}

	unsigned int MovieImpl::getChannelCount() const
	{
		if (m_demuxer && m_timer) {
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			if (audioStream)
				return audioStream->getChannelCount();
		}

		sfeLogError("Movie::getChannelCount() - No selected audio stream or no media loaded, cannot return a channel count");
		return 0;
	}

	Status MovieImpl::getStatus() const
	{
		Status st = Stopped;

		if (m_demuxer) {
			VideoStream* videoStream = m_demuxer->getSelectedVideoStream();
			AudioStream* audioStream = m_demuxer->getSelectedAudioStream();
			SubtitleStream* subtitleStream = m_demuxer->getSelectedSubtitleStream();
			Status vStatus = videoStream ? videoStream->getStatus() : Stopped;
			Status aStatus = audioStream ? audioStream->Stream::getStatus() : Stopped;
			Status sStatus = subtitleStream ? subtitleStream->getStatus() : Stopped;

			if (vStatus == Playing || aStatus == Playing || sStatus == Playing) {
				st = Playing;
			}
			else if (vStatus == Paused || aStatus == Paused || sStatus == Paused) {
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

		sfeLogError("Movie::getPlayingOffset() - No media loaded, cannot return a playing offset");
		return sf::Time::Zero;
	}

	const sf::Texture& MovieImpl::getCurrentImage() const
	{
		static sf::Texture emptyTexture;

		if (m_videoSprite.getTexture()) {
			return *m_videoSprite.getTexture();
		}
		else {
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
		
		target.draw(m_videoSprite, states);
		for (uint32_t i = 0; i < m_subtitleSprites.size(); ++i)
		{
			target.draw(m_subtitleSprites[i], states);
            sfeLogWarning("draw sub");
		}
	}
	
	void MovieImpl::didUpdateVideo(const VideoStream& sender, const sf::Texture& image)
	{
		if (m_videoSprite.getTexture() != &image) {
			m_videoSprite.setTexture(image);
		}
	}

	void MovieImpl::didUpdateSubtitle(const SubtitleStream& sender, const std::vector<sf::Sprite>& subs,
                                      const std::vector<sf::Vector2u>& subSizes)
	{
		m_subtitleSprites = subs;

        sf::Vector2f subtitlesCenter(m_displayFrame.width / 2, m_displayFrame.height * 0.9);
        
		for (uint32_t i = 0; i < m_subtitleSprites.size(); ++i)
		{
            sf::Sprite& subtitleSprite = m_subtitleSprites[i];
            subtitleSprite.setPosition(subtitlesCenter.x - (subSizes[i].x * m_scaleX / 2),
                                       subtitlesCenter.y - (subSizes[i].y * m_scaleY / 2));
            subtitleSprite.setScale(m_scaleX, m_scaleY);
		}
	}
}
