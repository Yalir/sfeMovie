
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <sfeMovie/Movie.hpp>
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

std::string StatusToString(sfe::Status status)
{
	switch (status) {
		case sfe::Stopped: return "Stopped"; break;
		case sfe::Paused: return "Paused"; break;
		case sfe::Playing: return "Playing"; break;
		default: return "unknown status"; break;
	}
}

std::string MediaTypeToString(sfe::MediaType type)
{
	switch (type) {
		case sfe::MEDIA_TYPE_AUDIO:		return "audio";
		case sfe::MEDIA_TYPE_SUBTITLE:	return "subtitle";
		case sfe::MEDIA_TYPE_VIDEO:		return "video";
		case sfe::MEDIA_TYPE_UNKNOWN:	return "unknown";
		default:						return "(null)";
	}
}

void drawControls(sf::RenderWindow& window, const sfe::Movie& movie)
{
	const int kHorizontalMargin = 10;
	const int kVerticalMargin = 10;
	const int kTimelineBackgroundHeight = 20;
	const int kTimelineInnerMargin = 4;
	
	sf::RectangleShape background(sf::Vector2f(window.getSize().x - 2 * kHorizontalMargin, kTimelineBackgroundHeight));
	background.setPosition(kHorizontalMargin, window.getSize().y - kTimelineBackgroundHeight - kVerticalMargin);
	background.setFillColor(sf::Color(0, 0, 0, 255/2));
	
	sf::RectangleShape border(sf::Vector2f(background.getSize().x - 2 * kTimelineInnerMargin, background.getSize().y - 2 * kTimelineInnerMargin));
	border.setPosition(background.getPosition().x + kTimelineInnerMargin, background.getPosition().y + kTimelineInnerMargin);
	border.setFillColor(sf::Color::Transparent);
	border.setOutlineColor(sf::Color::White);
	border.setOutlineThickness(1.0);
	
	float fprogress = movie.getPlayingOffset().asSeconds() / movie.getDuration().asSeconds();
	sf::RectangleShape progress(sf::Vector2f(1, border.getSize().y - 2 * border.getOutlineThickness()));
	progress.setPosition(border.getPosition().x + border.getOutlineThickness() + fprogress * (border.getSize().x - 2 * border.getOutlineThickness()), border.getPosition().y + border.getOutlineThickness());
	progress.setFillColor(sf::Color::White);
	
	window.draw(background);
	window.draw(border);
	window.draw(progress);
}

void printMovieInfo(const sfe::Movie& movie)
{
	std::cout << "Status: " << StatusToString(movie.getStatus()) << std::endl;
	std::cout << "Position: " << movie.getPlayingOffset().asSeconds() << "s" << std::endl;
	std::cout << "Duration: " << movie.getDuration().asSeconds() << "s" << std::endl;
	std::cout << "Size: " << movie.getSize().x << "x" << movie.getSize().y << std::endl;
	std::cout << "Framerate: " << movie.getFramerate() << " FPS (average)" << std::endl;
	std::cout << "Volume: " << movie.getVolume() << std::endl;
	std::cout << "Sample rate: " << movie.getSampleRate() << std::endl;
	std::cout << "Channel count: " << movie.getChannelCount() << std::endl;
	
	const std::vector<sfe::StreamDescriptor>& streams = movie.getStreams();
	std::cout << streams.size() << " streams found in the media" << std::endl;
	
	for (std::vector<sfe::StreamDescriptor>::const_iterator it = streams.begin(); it != streams.end(); ++it) {
		std::cout << " #" << it->index << " : " << MediaTypeToString(it->type);
		
		if (!it->language.empty())
			std::cout << " (language: " << it->language << ")";
		std::cout << std::endl;
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

	sfe::Movie movie;
	if (!movie.openFromFile(mediaFile)) {
		my_pause();
		return 1;
	}
	
	bool fullscreen = false;
	sf::VideoMode mode = sf::VideoMode::getDesktopMode();
	int width = std::min((int)mode.width, movie.getSize().x);
	int height = std::min((int)mode.height, movie.getSize().y);
	
	// Create window
	sf::RenderWindow window(sf::VideoMode(width, height), "sfeMovie Player", sf::Style::Close);
	movie.fit(0, 0, width, height);
	
	// Scale movie to the window drawing area and enable VSync
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
					if (movie.getStatus() == sfe::Playing) {
						movie.pause();
					} else {
						movie.play();
					}
				} else if (ev.key.code == sf::Keyboard::F) {
					fullscreen = !fullscreen;
					window.create(sf::VideoMode(width, height), "sfeMovie Player", fullscreen ? sf::Style::Fullscreen : sf::Style::Close);
					movie.fit(0, 0, window.getSize().x, window.getSize().y);
				} else if (ev.key.code == sf::Keyboard::P) {
					printMovieInfo(movie);
				}
			} else if (ev.type == sf::Event::MouseWheelMoved) {
				float volume = movie.getVolume() + 10 * ev.mouseWheel.delta;
				volume = std::min(volume, 100.f);
				volume = std::max(volume, 0.f);
				movie.setVolume(volume);
			}
		}
		
		movie.update();
		
		// Render movie
		window.clear();
		window.draw(movie);
		drawControls(window, movie);
		window.display();
	}
	
	return 0;
}

