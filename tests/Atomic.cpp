#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE Atomic
#include <boost/test/unit_test.hpp>
#include <atomic>
#include <iostream>
//boost test didn't allow a multithreaded example...

BOOST_AUTO_TEST_CASE(Atomic)
{
    std::atomic<int> num(18);
	BOOST_CHECK(num == 18);
}
