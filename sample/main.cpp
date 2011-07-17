
#include "Movie.h"
#include <SFML/Graphics.hpp>
#include <iostream>

#define MOVIE_FILE "some_movie.avi"

int main()
{
	// Create window and movie
	sf::RenderWindow window(sf::VideoMode(800, 600), "SFE Movie Player", sf::Style::Close);
	sfe::Movie movie;
	bool fullscreen = false;
	
	if (!movie.OpenFromFile(MOVIE_FILE))
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
				//movie.Stop();
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
				
				if (ev.Key.Code == sf::Key::F)
				{
					fullscreen = !fullscreen;
					
					if (fullscreen)
					{
						window.Close();
						window.Create(sf::VideoMode::GetDesktopMode(), "SFE Movie Player", sf::Style::Fullscreen);
						window.EnableVerticalSync(true);
						movie.ResizeToFrame(0, 0, window.GetWidth(), window.GetHeight());
					}
					else
					{
						window.Close();
						window.Create(sf::VideoMode(1024, 768), "SFE Movie Player", sf::Style::Close);
						window.EnableVerticalSync(true);
						movie.ResizeToFrame(0, 0, window.GetWidth(), window.GetHeight());
					}
				}
				
				if (ev.Key.Code ==sf::Key::Escape)
				{
					//movie.Stop();
					window.Close();
				}
				
				if (ev.Key.Code == sf::Key::R)
				{
					movie.Stop();
					movie.Play();
				}
			}
		}
		
		// Render movie
		window.Clear();
		window.Draw(movie);
		window.Display();
	}

	return 0;
}

