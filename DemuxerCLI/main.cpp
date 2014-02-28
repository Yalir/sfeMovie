
#include <SFML/Config.hpp>
#include "Demuxer.hpp"
#include "Utilities.hpp"
#include <iostream>

void my_pause()
{
#ifdef SFML_SYSTEM_WINDOWS
	system("PAUSE");
#endif
}

int main(int argc, const char *argv[])
{
	// Some settings
	
	if (argc < 2)
	{
		std::cout << "Usage: " << std::string(argv[0]) << " media_path" << std::endl;
		my_pause();
		return 1;
	}
	
	std::string mediaFile = std::string(argv[1]);
	std::cout << "Going to open movie file \"" << mediaFile << "\"" << std::endl;
	
	sfe::dumpAvailableDecoders();
	
	sfe::Timer timer;
	sfe::Demuxer demuxer(mediaFile, timer);
	timer.play();
	
	while (!demuxer.didReachEndOfFile())
		sf::sleep(sf::milliseconds(100));
	
	return 0;
}

