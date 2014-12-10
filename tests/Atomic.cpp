#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE Atomic
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <iostream>
//boost test didn't allow a multithreaded example...

BOOST_AUTO_TEST_CASE(Atomic)
{
	std::atomic<int> num;
	num = 0;
	for(int i = 0;i<100;++i)
	{
	++num;
	}
	std::cout<< num << std::endl;
	BOOST_CHECK(num == 100);
}
