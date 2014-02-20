
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE DemuxerTest
#include <boost/test/unit_test.hpp>
#include "Demuxer.hpp"

BOOST_AUTO_TEST_CASE(DemuxerLoadingTest)
{
	sfe::Demuxer demuxer("small.ogv");
}
