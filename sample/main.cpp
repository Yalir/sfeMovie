
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include "Demuxer.hpp"
#include "Utilities.hpp"
#include "Log.hpp"
#include "VideoStream.hpp"
#include <iostream>

/*
 * Here is a little use sample for sfeMovie.
 * It'll open and display the movie specified by MOVIE_FILE above.
 *
 * This sample implements basic controls as follow:
 *  - Escape key to exit
 *  - Space key to play/pause the movie playback
 *  - S key to stop and go back to the beginning of the movie
 *  - R key to restart playing from the beginning of the movie
 *  - F key to toggle between windowed and fullscreen mode
 */

void my_pause()
{
#ifdef SFML_SYSTEM_WINDOWS
	system("PAUSE");
#endif
}

int main(int argc, const char *argv[])
{
	// Some settings
	const std::string windowTitle = "sfeMovie Player";
	const int windowWidth = 1280;
	const int windowHeight = 800;
	
	if (argc < 2)
	{
		std::cout << "Usage: " << std::string(argv[0]) << " movie_path" << std::endl;
		my_pause();
		return 1;
	}
	
	std::string mediaFile = std::string(argv[1]);
	std::cout << "Going to open movie file \"" << mediaFile << "\"" << std::endl;
	
	// Create window
	sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), windowTitle, sf::Style::Close);
	
	sfe::dumpAvailableDecoders();
	sfe::Log::setMask(sfe::Log::DebugMask | sfe::Log::WarningMask);
	
	sfe::Timer timer;
	sfe::Demuxer demuxer(mediaFile, timer);
	
	const std::map<int, sfe::Stream*>& vStreams = demuxer.getStreams();
	sfe::VideoStream* vStream = NULL;
	
	if (vStreams.size()) {
		vStream = dynamic_cast<sfe::VideoStream*>(vStreams.begin()->second);
	}
	
	timer.play();
	
	sf::Sprite sp;
	
	if (vStream) {
		sp.setTexture(vStream->getVideoTexture());
	}
	
	// Scale movie to the window drawing area and enable VSync
	window.setVerticalSyncEnabled(true);

	while (window.isOpen() && !demuxer.didReachEndOfFile())
	{
		sf::Event ev;
		while (window.pollEvent(ev))
		{
			// Window closure
			if (ev.type == sf::Event::Closed ||
				(ev.type == sf::Event::KeyPressed &&
				 ev.key.code == sf::Keyboard::Escape))
			{
				window.close();
			}
		}
		
		demuxer.updateVideoStreams();
		
		// Render movie
		window.clear();
		window.draw(sp);
		window.display();
	}

//	timer.stop();
	
	return 0;
}

