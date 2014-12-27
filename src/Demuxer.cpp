
/*
 *  Demuxer.cpp
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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <stdint.h>
}

#include "Demuxer.hpp"
#include "VideoStream.hpp"
#include "AudioStream.hpp"
#include "Log.hpp"
#include "Utilities.hpp"
#include "TimerPriorities.hpp"
#include <iostream>
#include <stdexcept>

namespace sfe
{
    std::list<Demuxer::DemuxerInfo> Demuxer::g_availableDemuxers;
    std::list<Demuxer::DecoderInfo> Demuxer::g_availableDecoders;
    
    static void loadFFmpeg()
    {
        ONCE(av_register_all());
        ONCE(avcodec_register_all());
        ONCE(Log::initialize());
    }
    
    static MediaType AVMediaTypeToMediaType(AVMediaType type)
    {
        switch (type)
        {
            case AVMEDIA_TYPE_AUDIO:    return Audio;
            case AVMEDIA_TYPE_SUBTITLE:     return Subtitle;
            case AVMEDIA_TYPE_VIDEO:    return Video;
            default:                    return Unknown;
        }
    }
    
    const std::list<Demuxer::DemuxerInfo>& Demuxer::getAvailableDemuxers()
    {
        AVInputFormat* demuxer = nullptr;
        loadFFmpeg();
        
        if (g_availableDemuxers.empty())
        {
            while (nullptr != (demuxer = av_iformat_next(demuxer)))
            {
                DemuxerInfo info =
                {
                    std::string(demuxer->name),
                    std::string(demuxer->long_name)
                };
                
                g_availableDemuxers.push_back(info);
            }
        }
        
        return g_availableDemuxers;
    }
    
    const std::list<Demuxer::DecoderInfo>& Demuxer::getAvailableDecoders()
    {
        AVCodec* codec = nullptr;
        loadFFmpeg();
        
        if (g_availableDecoders.empty())
        {
            while (nullptr != (codec = av_codec_next(codec)))
            {
                DecoderInfo info =
                {
                    avcodec_get_name(codec->id),
                    codec->long_name,
                    AVMediaTypeToMediaType(codec->type)
                };
                
                g_availableDecoders.push_back(info);
            }
        }
        
        return g_availableDecoders;
    }
    
    Demuxer::Demuxer(const std::string& sourceFile, std::shared_ptr<Timer> timer,
                     VideoStream::Delegate& videoDelegate, SubtitleStream::Delegate& subtitleDelegate) :
    m_formatCtx(nullptr),
    m_eofReached(false),
    m_streams(),
    m_ignoredStreams(),
    m_synchronized(),
    m_timer(timer),
    m_connectedAudioStream(nullptr),
    m_connectedVideoStream(nullptr),
    m_connectedSubtitleStream(nullptr),
    m_duration(sf::Time::Zero)
    {
        CHECK(sourceFile.size(), "Demuxer::Demuxer() - invalid argument: sourceFile");
        CHECK(timer, "Inconsistency error: null timer");
        
        int err = 0;
        
        // Load all the decoders
        loadFFmpeg();
        
        // Open the movie file
        err = avformat_open_input(&m_formatCtx, sourceFile.c_str(), nullptr, nullptr);
        CHECK0(err, "Demuxer::Demuxer() - error while opening media: " + sourceFile);
        CHECK(m_formatCtx, "Demuxer() - inconsistency: media context cannot be nullptr");
        
        // Read the general movie informations
        err = avformat_find_stream_info(m_formatCtx, nullptr);
        CHECK0(err, "Demuxer::Demuxer() - error while retreiving media information");
        
        // Get the media duration if possible (otherwise rely on the streams)
        if (m_formatCtx->duration != AV_NOPTS_VALUE)
        {
            int64_t secs, us;
            secs = m_formatCtx->duration / AV_TIME_BASE;
            us = m_formatCtx->duration % AV_TIME_BASE;
            m_duration = sf::seconds(secs + (float)us / AV_TIME_BASE);
        }
        
        // Find all interesting streams
        for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++)
        {
            AVStream* & ffstream = m_formatCtx->streams[i];
            
            try
            {
                std::shared_ptr<Stream> stream;
                
                switch (ffstream->codec->codec_type)
                {
                    case AVMEDIA_TYPE_VIDEO:
                        stream = std::make_shared<VideoStream>(m_formatCtx, ffstream, *this, timer, videoDelegate);
                        
                        if (m_duration == sf::Time::Zero)
                        {
                            extractDurationFromStream(ffstream);
                        }
                        
                        sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " video stream");
                        break;
                        
                    case AVMEDIA_TYPE_AUDIO:
                        stream = std::make_shared<AudioStream>(m_formatCtx, ffstream, *this, timer);
                        
                        if (m_duration == sf::Time::Zero)
                        {
                            extractDurationFromStream(ffstream);
                        }
                        
                        sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " audio stream");
                        break;
                    case AVMEDIA_TYPE_SUBTITLE:
                        stream = std::make_shared<SubtitleStream>(m_formatCtx, ffstream, *this, timer, subtitleDelegate);
                        
                        sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " subtitle stream");
                        break;
                    default:
                        m_ignoredStreams[ffstream->index] = Stream::AVStreamDescription(ffstream);
                        sfeLogDebug(m_ignoredStreams[ffstream->index] + " ignored");
                        break;
                }
                
                // Don't create an entry in the map unless everything went well and stream did not get ignored
                if (stream)
                    m_streams[ffstream->index] = stream;
            }
            catch (std::runtime_error& e)
            {
                std::string streamDesc = Stream::AVStreamDescription(ffstream);
                
                sfeLogError("error while loading " + streamDesc + ": " + e.what());
                CHECK(m_streams.find(ffstream->index) == m_streams.end(),
                      "Internal inconcistency error: stream whose loading failed should not be stored");
            }
        }
        
        if (m_duration == sf::Time::Zero)
        {
            sfeLogWarning("The media duration could not be retreived");
        }
        
        m_timer->addObserver(*this, DemuxerTimerPriority);
    }
    
    Demuxer::~Demuxer()
    {
        if (m_timer->getStatus() != Stopped)
            m_timer->stop();
        
        m_timer->removeObserver(*this);
        
        if (m_formatCtx)
        {
            // Be very careful with this call: it'll also destroy its codec contexts and streams
            avformat_close_input(&m_formatCtx);
        }
        
        flushBuffers();
    }
    
    const std::map<int, std::shared_ptr<Stream> >& Demuxer::getStreams() const
    {
        return m_streams;
    }
    
    std::set< std::shared_ptr<Stream> > Demuxer::getStreamsOfType(MediaType type) const
    {
        std::set< std::shared_ptr<Stream> > streamSet;
        
        for (const std::pair<int, std::shared_ptr<Stream> >& pair : m_streams)
        {
            if (pair.second->getStreamKind() == type)
                streamSet.insert(pair.second);
        }
        
        return streamSet;
    }
    
    Streams Demuxer::computeStreamDescriptors(MediaType type) const
    {
        Streams entries;
        std::set< std::shared_ptr<Stream> > streamSet;

        for (const std::pair<int, std::shared_ptr<Stream> >& pair : m_streams)
        {
            if (pair.second->getStreamKind() == type)
            {
                StreamDescriptor entry;
                entry.type = type;
                entry.identifier = pair.first;
                entry.language = pair.second->getLanguage();
                entries.push_back(entry);
            }
        }
        
        return entries;
    }
    
    void Demuxer::selectAudioStream(std::shared_ptr<AudioStream> stream)
    {
        Status oldStatus = m_timer->getStatus();
        CHECK(oldStatus == Stopped, "Changing the selected stream after starting "
              "the movie playback isn't supported yet");
        
        if (oldStatus == Playing)
            m_timer->pause();
        
        if (stream != m_connectedAudioStream)
        {
            if (m_connectedAudioStream)
            {
                m_connectedAudioStream->disconnect();
            }
            
            if (stream)
                stream->connect();
            
            m_connectedAudioStream = stream;
        }
        
        if (oldStatus == Playing)
            m_timer->play();
    }
    
    void Demuxer::selectFirstAudioStream()
    {
        std::set< std::shared_ptr<Stream> > audioStreams = getStreamsOfType(Audio);
        if (audioStreams.size())
            selectAudioStream(std::dynamic_pointer_cast<AudioStream>(*audioStreams.begin()));
    }
    
    std::shared_ptr<AudioStream> Demuxer::getSelectedAudioStream() const
    {
        return std::dynamic_pointer_cast<AudioStream>(m_connectedAudioStream);
    }
    
    void Demuxer::selectVideoStream(std::shared_ptr<VideoStream> stream)
    {
        Status oldStatus = m_timer->getStatus();
        CHECK(oldStatus == Stopped, "Changing the selected stream after starting "
              "the movie playback isn't supported yet");
        
        if (oldStatus == Playing)
            m_timer->pause();
        
        if (stream != m_connectedVideoStream)
        {
            if (m_connectedVideoStream)
            {
                m_connectedVideoStream->disconnect();
            }
            
            if (stream)
                stream->connect();
            
            m_connectedVideoStream = stream;
        }
        
        if (oldStatus == Playing)
            m_timer->play();
    }
    
    void Demuxer::selectFirstVideoStream()
    {
        std::set< std::shared_ptr<Stream> > videoStreams = getStreamsOfType(Video);
        if (videoStreams.size())
            selectVideoStream(std::dynamic_pointer_cast<VideoStream>(*videoStreams.begin()));
    }
    
    std::shared_ptr<VideoStream> Demuxer::getSelectedVideoStream() const
    {
        return std::dynamic_pointer_cast<VideoStream>(m_connectedVideoStream);
    }
    
    void Demuxer::selectSubtitleStream(std::shared_ptr<SubtitleStream> stream)
    {
        Status oldStatus = m_timer->getStatus();
        
        if (oldStatus == Playing)
            m_timer->pause();
        
        if (stream != m_connectedSubtitleStream)
        {
            if (m_connectedSubtitleStream)
                m_connectedSubtitleStream->disconnect();
            
            if (stream)
                stream->connect();
            
            m_connectedSubtitleStream = stream;
        }
        
        if (oldStatus == Playing)
            m_timer->play();
    }
    
    void Demuxer::selectFirstSubtitleStream()
    {
        std::set< std::shared_ptr<Stream> > subtitleStreams = getStreamsOfType(Subtitle);
        if (subtitleStreams.size())
            selectSubtitleStream(std::dynamic_pointer_cast<SubtitleStream>(*subtitleStreams.begin()));
    }
    
    std::shared_ptr<SubtitleStream> Demuxer::getSelectedSubtitleStream() const
    {
        return std::dynamic_pointer_cast<SubtitleStream>(m_connectedSubtitleStream);
    }
    
    void Demuxer::feedStream(Stream& stream)
    {
        sf::Lock l(m_synchronized);
        
        while (!didReachEndOfFile() && stream.needsMoreData())
        {
            AVPacket* pkt = NULL;
            
            pkt = gatherQueuedPacketForStream(stream);
            
            if (!pkt)
                pkt = readPacket();
            
            if (!pkt)
            {
                m_eofReached = true;
            }
            else
            {
                if (!distributePacket(pkt, stream))
                {
                    AVStream* ffstream = m_formatCtx->streams[pkt->stream_index];
                    std::string streamName = Stream::AVStreamDescription(ffstream);
                    
                    sfeLogDebug(streamName + ": packet dropped");
                    av_free_packet(pkt);
                    av_free(pkt);
                }
            }
        }
    }
    
    void Demuxer::update()
    {
        std::map<int, std::shared_ptr<Stream> > streams = getStreams();
        std::map<int, std::shared_ptr<Stream> >::iterator it;
        
        for(std::pair<int, std::shared_ptr<Stream> > pair : streams)
        {
			pair.second->update();
		}
    }
    
    bool Demuxer::didReachEndOfFile() const
    {
        return m_eofReached;
    }
    
    sf::Time Demuxer::getDuration() const
    {
        return m_duration;
    }
    
    AVPacket* Demuxer::readPacket()
    {
        sf::Lock l(m_synchronized);
        
        AVPacket *pkt = nullptr;
        int err = 0;
        
        pkt = (AVPacket *)av_malloc(sizeof(*pkt));
        CHECK(pkt, "Demuxer::readPacket() - out of memory");
        av_init_packet(pkt);
        
        err = av_read_frame(m_formatCtx, pkt);
        
        if (err < 0)
        {
            av_free_packet(pkt);
            av_free(pkt);
            pkt = nullptr;
        }
        
        return pkt;
    }
    
    void Demuxer::flushBuffers()
    {
        sf::Lock l(m_synchronized);
        
        for (AVPacket* packet : m_pendingDataForActiveStreams)
        {
            av_free_packet(packet);
            av_free(packet);
        }
        
        m_pendingDataForActiveStreams.clear();
    }
    
    void Demuxer::queueEncodedData(AVPacket* packet)
    {
        sf::Lock l(m_synchronized);
        m_pendingDataForActiveStreams.push_back(packet);
    }
    
    AVPacket* Demuxer::gatherQueuedPacketForStream(Stream& stream)
    {
        sf::Lock l(m_synchronized);
        for (std::list<AVPacket*>::iterator it = m_pendingDataForActiveStreams.begin();
             it != m_pendingDataForActiveStreams.end(); ++it)
        {
            AVPacket* packet = *it;
            
            if (stream.canUsePacket(packet))
            {
                m_pendingDataForActiveStreams.erase(it);
                return packet;
            }
        }
        
        return NULL;
    }
    
    bool Demuxer::distributePacket(AVPacket* packet, Stream& stream)
    {
        sf::Lock l(m_synchronized);
        CHECK(packet, "Demuxer::distributePacket() - invalid argument");
        
        bool distributed = false;
        std::map<int, std::shared_ptr<Stream> >::iterator it = m_streams.find(packet->stream_index);
        
        if (it != m_streams.end())
        {
             std::shared_ptr<Stream>  targetStream = it->second;
            
            // We don't want to store the packets for inactive streams,
            // let them be freed
            if (targetStream == getSelectedVideoStream() ||
                targetStream == getSelectedAudioStream() ||
                targetStream == getSelectedSubtitleStream())
            {
                if (targetStream.get() == &stream || targetStream->isPassive())
                    targetStream->pushEncodedData(packet);
                else
                    queueEncodedData(packet);
                
                distributed = true;
            }
        }
        
        return distributed;
    }
    
    void Demuxer::extractDurationFromStream(const AVStream* stream)
    {
        if (m_duration != sf::Time::Zero)
            return;
        
        if (stream->duration != AV_NOPTS_VALUE)
        {
            int64_t secs, us;
            secs = stream->duration / AV_TIME_BASE;
            us = stream->duration % AV_TIME_BASE;
            m_duration = sf::seconds(secs + (float)us / AV_TIME_BASE);
        }
    }
    
    void Demuxer::requestMoreData(Stream& starvingStream)
    {
        sf::Lock l(m_synchronized);
        
        feedStream(starvingStream);
    }
    
    void Demuxer::resetEndOfFileStatus()
    {
        m_eofReached = false;
    }
    
    void Demuxer::didSeek(const Timer &timer, sf::Time oldPosition)
    {
        resetEndOfFileStatus();
        sf::Time newPosition = timer.getOffset();
        std::set< std::shared_ptr<Stream> > connectedStreams;
        
        if (m_connectedVideoStream)
            connectedStreams.insert(m_connectedVideoStream);
        if (m_connectedAudioStream)
            connectedStreams.insert(m_connectedAudioStream);
        if (m_connectedSubtitleStream)
            connectedStreams.insert(m_connectedSubtitleStream);
        
        CHECK(connectedStreams.size() > 0, "Inconcistency error: seeking with no active stream");
        
        // Trivial seeking to beginning
        if (newPosition == sf::Time::Zero)
        {
            int64_t timestamp = 0;
            
            if (m_formatCtx->iformat->flags & AVFMT_SEEK_TO_PTS && m_formatCtx->start_time != AV_NOPTS_VALUE)
                timestamp += m_formatCtx->start_time;
            
            
            // Flush all streams
            for (std::shared_ptr<Stream> stream : connectedStreams)
                stream->flushBuffers();
            flushBuffers();
            
            // Seek to beginning
            int err = avformat_seek_file(m_formatCtx, -1, INT64_MIN, timestamp, INT64_MAX, AVSEEK_FLAG_BACKWARD);
            if (err < 0)
                sfeLogError("Error while seeking at time " + s(newPosition.asMilliseconds()) + "ms");
        }
        else // Seeking to some other position
        {
            // Initial target seek point
            int64_t timestamp = newPosition.asSeconds() * AV_TIME_BASE;
            
            // < 0 = before seek point
            // > 0 = after seek point
            std::map< std::shared_ptr<Stream>, sf::Time> seekingGaps;
            
            static const float tooEarlyThreshold = 10.f; // seconds
            static const float brokenSeekingThreshold = 2 * tooEarlyThreshold;
            bool didReseekBackward = false;
            bool didReseekForward = false;
            int tooEarlyCount = 0;
            int tooLateCount = 0;
            int brokenSeekingCount = 0;
            
            do
            {
                // Flush all streams
                for (std::shared_ptr<Stream> stream : connectedStreams)
                    stream->flushBuffers();
                flushBuffers();
                
                // Seek to new estimated target
                if (m_formatCtx->iformat->flags & AVFMT_SEEK_TO_PTS && m_formatCtx->start_time != AV_NOPTS_VALUE)
                    timestamp += m_formatCtx->start_time;
                
                int err = avformat_seek_file(m_formatCtx, -1, timestamp - 10 * AV_TIME_BASE, timestamp, timestamp,
                                             AVSEEK_FLAG_BACKWARD);
                CHECK0(err, "avformat_seek_file failure");
                
                // Compute the new gap
                for (std::shared_ptr<Stream> stream : connectedStreams)
                {
                    sf::Time gap = stream->computePosition() - newPosition;
                    seekingGaps[stream] = gap;
        }
                
                tooEarlyCount = 0;
                tooLateCount = 0;
                brokenSeekingCount = 0;
                
                // Check the current situation
                for (std::pair< std::shared_ptr<Stream>, sf::Time>&& gapByStream : seekingGaps)
                {
                    // < 0 = before seek point
                    // > 0 = after seek point
                    const sf::Time& gap = gapByStream.second;
                    float absoluteDiff = fabs(gap.asSeconds());
                    
                    // Before seek point
                    if (gap < sf::Time::Zero)
                    {
                        if (absoluteDiff > brokenSeekingThreshold)
                        {
                        brokenSeekingCount++;
                            tooEarlyCount++;
    }
                        else if (absoluteDiff > tooEarlyThreshold)
                        tooEarlyCount++;
                    
                        // else: a bit early but not too much, this is the final situation we want
}
                    // After seek point
                    else if (gap > sf::Time::Zero)
                    {
                        tooLateCount++;
                    
                        if (absoluteDiff > brokenSeekingThreshold)
                        brokenSeekingCount++; // TODO: unhandled for now => should seek to non-key frame
                    }
                    
                    if (brokenSeekingCount > 0)
                        sfeLogWarning("Seeking on " + gapByStream.first->description() + " is broken! Gap: "
                                      + s(gap.asSeconds()) + "s");
                }
                
                CHECK(false == (tooEarlyCount && tooLateCount),
                      "Both too late and too early for different streams, unhandled situation!");
                
                // Define what to do next
                if (tooEarlyCount)
                {
                    // Go forward by 1 sec
                    timestamp += AV_TIME_BASE;
                    didReseekForward = true;
                }
                else if (tooLateCount)
                {
                    // Go backward by 1 sec
                    timestamp -= AV_TIME_BASE;
                    didReseekBackward = true;
                }
                
                CHECK(!(didReseekBackward && didReseekForward), "infinitely seeking backward and forward");
            }
            while (tooEarlyCount != 0 || tooLateCount != 0);
        }
    }
}
