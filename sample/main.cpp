
#include "../include/Movie.h"
#include <SFML/Graphics.hpp>

#ifndef MOVIE_FILE
#define MOVIE_FILE "some_movie.avi"
#endif

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

int main()
{
	// Some settings
	const std::string windowTitle = "sfeMovie Player";
	const int windowWidth = 800;
	const int windowHeight = 600;
	bool fullscreen = false;
	
	// Create window
	sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), windowTitle, sf::Style::Close);
	
	// Create and open movie
	sfe::Movie movie;
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
			// Window closure
			if (ev.Type == sf::Event::Closed ||
				(ev.Type == sf::Event::KeyPressed &&
				 ev.Key.Code == sf::Keyboard::Escape))
			{
				window.Close();
			}
			
			// Handle basic controls
			else if (ev.Type == sf::Event::KeyPressed)
			{
				// Play/Pause
				if (ev.Key.Code == sf::Keyboard::Space)
				{
					if (movie.GetStatus() != sfe::Movie::Playing)
						movie.Play();
					else
						movie.Pause();
				}
				
				// Stop
				if (ev.Key.Code == sf::Keyboard::S)
					movie.Stop();
				
				// Restart playback
				if (ev.Key.Code == sf::Keyboard::R)
				{
					movie.Stop();
					movie.Play();
				}
				
				// Toggle fullscreen mode
				if (ev.Key.Code == sf::Keyboard::F)
				{
					fullscreen = !fullscreen;
					
					// We want to switch to the full screen mode
					if (fullscreen)
					{
						window.Create(sf::VideoMode::GetDesktopMode(), windowTitle, sf::Style::Fullscreen);
						window.EnableVerticalSync(true);
						movie.ResizeToFrame(0, 0, window.GetWidth(), window.GetHeight());
					}
					
					// We want to switch back to the windowed mode
					else
					{
						window.Create(sf::VideoMode(windowWidth, windowHeight), windowTitle, sf::Style::Close);
						window.EnableVerticalSync(true);
						movie.ResizeToFrame(0, 0, window.GetWidth(), window.GetHeight());
					}
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

