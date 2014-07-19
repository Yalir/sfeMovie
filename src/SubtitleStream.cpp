extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <cstring>
#include <iostream>
#include "SubtitleStream.hpp"
#include "Log.hpp"
#include <sfeMovie/Movie.hpp>

namespace sfe {
	SubtitleStream::SubtitleStream(AVFormatContext* formatCtx, AVStream* stream, DataSource& dataSource, Timer& timer) :
		Stream(formatCtx, stream, dataSource, timer)
	{
	}

	/** Default destructor
	*/
	SubtitleStream::~SubtitleStream()
	{

	}

	MediaType SubtitleStream::getStreamKind() const
	{
		return MEDIA_TYPE_SUBTITLE;
	}

	void SubtitleStream::update()
	{
		//only get new subtitles if we are running low
		if (m_status == Status::Playing && m_inactive.size()<m_buffersize)
		{
			if (!onGetData(m_output)) {
				setStatus(Stopped);
			}
		}

		for (int i = 0; i < m_inactive.size(); ++i)
		{
			//activate subtitle
			if (m_inactive[i]->pts <= m_timer.getOffset().asMilliseconds())
			{
				m_current = m_inactive[i];
				m_inactive.erase(m_inactive.begin() + i);
				

				for (int j = 0; j < m_current->sub.num_rects; ++j)
				{				

					if (m_current->sub.rects[j]->type == SUBTITLE_BITMAP)
					{
						for (int k = 0; k < m_current->sub.rects[j]->nb_colors; ++k)
						{
							sf::Texture tex;
							tex.create(m_current->sub.rects[j]->w, m_current->sub.rects[j]->h);
							AVPicture* rgba_data = ARGBtoRGBA(&m_current->sub.rects[j]->pict, m_current->sub.rects[j]->w, m_current->sub.rects[j]->h);
							tex.update(m_current->sub.rects[j]->pict.data[1] + k);
							av_freep(&rgba_data);
							m_current->out.push_back(tex);
						}
						
					}
				}

				m_active.push_back(m_current);
			}
		}
	}

	bool SubtitleStream::onGetData(sf::Texture& texture)
	{
		AVPacket* packet = popEncodedData();
		AVSubtitle sub;
		int gotSub = 0;
		bool goOn = false;
		double pts = 0;

		if (packet) {
			goOn = true;

			while (!gotSub && packet && goOn) {
				bool needsMoreDecoding = false;

				CHECK(packet != NULL, "inconsistency error");
				goOn = avcodec_decode_subtitle2(m_codecCtx, &sub, &gotSub, packet);

				pts = 0;
				if (packet->pts != AV_NOPTS_VALUE)
					pts =  packet->pts;

				if (gotSub && pts) {
					SubImage* subimg = new SubImage();
					subimg->sub = sub;
					subimg->pts = pts;
					
					m_inactive.push_back(subimg);
				}

				if (needsMoreDecoding) {
					prependEncodedData(packet);
				}
				else {
					av_free_packet(packet);
					av_free(packet);
				}

				if (!gotSub && goOn) {
					sfeLogDebug("no image in this packet, reading further");
					packet = popEncodedData();
				}
			}
		}

		return goOn;
	}

	AVPicture* SubtitleStream::ARGBtoRGBA(AVPicture* input, int width, int height)
	{

		struct SwsContext *resize;
		resize = sws_getContext(width, height, PIX_FMT_PAL8, width, height, PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);

		AVFrame* output = avcodec_alloc_frame();
		int num_bytes = avpicture_get_size(PIX_FMT_RGBA, width, height);
		uint8_t* output_buffer = (uint8_t *)av_malloc(num_bytes*sizeof(uint8_t));
		avpicture_fill((AVPicture*)output, output_buffer, PIX_FMT_RGB24, width, height);
		sws_scale(resize, input->data, input->linesize, 0, height, output->data, output->linesize);
		return (AVPicture*)output;
	}
}