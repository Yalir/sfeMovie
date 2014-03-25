
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <sfeMovie/Movie.hpp>
#include "Utilities.hpp"
#include <iostream>
#include <algorithm>

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

std::string StatusToString(sfe::Movie::Status status)
{
	switch (status) {
		case sfe::Movie::Stopped: return "Stopped"; break;
		case sfe::Movie::Paused: return "Paused"; break;
		case sfe::Movie::Playing: return "Playing"; break;
		default: return "unknown status"; break;
	}
}

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: " << std::string(argv[0]) << " movie_path" << std::endl;
		my_pause();
		return 1;
	}
	
	std::string mediaFile = std::string(argv[1]);
	std::cout << "Going to open movie file \"" << mediaFile << "\"" << std::endl;

//	sfe::Log::setMask(sfe::Log::DebugMask | sfe::Log::WarningMask);
	sfe::Movie movie;
	
	if (!movie.openFromFile(mediaFile))
		return 1;
	
	bool fullscreen = false;
	sf::VideoMode mode = sf::VideoMode::getDesktopMode();
	int width = std::min((int)mode.width, movie.getSize().x);
	int height = std::min((int)mode.height, movie.getSize().y);
	
	// Create window
	sf::RenderWindow window(sf::VideoMode(width, height), "sfeMovie Player", sf::Style::Close);
	
	// Scale movie to the window drawing area and enable VSync
//	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);
	movie.play();

	while (window.isOpen())
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
			
			if (ev.type == sf::Event::KeyPressed) {
				if (ev.key.code == sf::Keyboard::Space) {
					if (movie.getStatus() == sfe::Movie::Playing) {
						movie.pause();
					} else {
						movie.play();
					}
				} else if (ev.key.code == sf::Keyboard::F) {
					fullscreen = !fullscreen;
					window.create(sf::VideoMode(width, height), "sfeMovie Player", fullscreen ? sf::Style::Fullscreen : sf::Style::Close);
				} else if (ev.key.code == sf::Keyboard::P) {
					std::cout << "Volume: " << movie.getVolume() << std::endl;
					std::cout << "Duration: " << movie.getDuration().asSeconds() << "s" << std::endl;
					std::cout << "Size: " << movie.getSize().x << "x" << movie.getSize().y << std::endl;
					std::cout << "Framerate: " << movie.getFramerate() << " FPS (average)" << std::endl;
					std::cout << "Sample rate: " << movie.getSampleRate() << std::endl;
					std::cout << "Channel count: " << movie.getChannelCount() << std::endl;
					std::cout << "Status: " << StatusToString(movie.getStatus()) << std::endl;
					std::cout << "Position: " << movie.getPlayingOffset().asSeconds() << "s" << std::endl;
				}
			}
		}
		
		movie.update();
		
		// Render movie
		window.clear();
		window.draw(movie);
		window.display();
	}

//	timer.stop();
	
	return 0;
}

