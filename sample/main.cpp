
#include <sfe/Movie.h>
#include <SFML/Graphics.hpp>
#include <iostream>

int main()
{
	sf::VideoMode mode = sf::VideoMode(640, 480);
    sf::RenderWindow window(mode, "SFE Movie Player", sf::Style::Close);

    sfe::Movie movie;
	movie.UseDebugMessages();
    if (!movie.OpenFromFile("/Hors Sauvegarde/sync-720p.mov"))
        return 1;


	movie.ResizeToFrame(0, 0, mode.Width, mode.Height);

	window.EnableVerticalSync(true);
	//window.ShowMouseCursor(false);
	//window.SetFramerateLimit(40);
	//window.EnableKeyRepeat(false);


    movie.Play();

    while (window.IsOpened())
    {
        sf::Event ev;
        while (window.GetEvent(ev))
        {
            if (ev.Type == sf::Event::Closed)
			{
				movie.Stop();
                window.Close();
			}

            if (ev.Type == sf::Event::KeyPressed)
			{
                if (ev.Key.Code == sf::Key::Escape)
				{
					movie.Stop();
					window.Close();
				}

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
		
        window.Clear();
        window.Draw(movie);
        window.Display();
    }

    return 0;
}

