
#include "Movie.h"
#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
	// Create window and movie
	sf::RenderWindow window(sf::VideoMode(1280, 720), "SFE Movie Player", sf::Style::Close);
	sfe::Movie movie;
	
	// Print debug messages (show file information and alert when movie playback is late)
	sfe::Movie::UseDebugMessages(true);
	if (!movie.OpenFromFile("sintel-1024-stereo.ogv"))
		return 1;

	// Scale movie to the window drawing area and enable VSync
	movie.ResizeToFrame(0, 0, window.GetWidth(), window.GetHeight());
	window.EnableVerticalSync(true);

	// Start movie playback
	movie.Play();

	while (window.IsOpened())
	{
		sf::Event ev;
		while (window.PollEvent(ev))
		{
			if (ev.Type == sf::Event::Closed ||
				(ev.Type == sf::Event::KeyPressed && ev.Key.Code == sf::Key::Escape))
			{
				movie.Stop();
				window.Close();
			}

			if (ev.Type == sf::Event::KeyPressed)
			{
				// Handle basic controls
				if (ev.Key.Code == sf::Key::Space)
				{
					if (movie.GetStatus() == sfe::Movie::Paused || movie.GetStatus() == sfe::Movie::Stopped)
						movie.Play();
					else
						movie.Pause();
				}

				if (ev.Key.Code == sf::Key::S)
					movie.Stop();
			}
		}
		
		// Render movie
		window.Clear();
		window.Draw(movie);
		window.Display();
	}

	return 0;
}

