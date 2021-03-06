/* +------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)            |
   |                          https://www.mrpt.org/                         |
   |                                                                        |
   | Copyright (c) 2005-2020, Individual contributors, see AUTHORS file     |
   | See: https://www.mrpt.org/Authors - All rights reserved.               |
   | Released under BSD License. See: https://www.mrpt.org/License          |
   +------------------------------------------------------------------------+ */

#include <gtest/gtest.h>
#include <mrpt/apps/RawlogGrabberApp.h>
#include <mrpt/config.h>
#include <mrpt/system/filesystem.h>
#include <test_mrpt_common.h>
#include <thread>

// Test disabled: it fails to run in build servers for some unknown reason (!)
// (JLBC May 2020, after debian package release 2:2.0.3-3)
#if MRPT_HAS_FFMPEG && MRPT_HAS_OPENCV && 0
TEST(RawlogGrabberApp, CGenericCamera_AVI)
#else
TEST(RawlogGrabberApp, DISABLED_CGenericCamera_AVI)
#endif
{
	using namespace std::string_literals;

	const std::string ini_fil =
		mrpt::UNITTEST_BASEDIR +
		"/share/mrpt/config_files/rawlog-grabber/camera_ffmpeg_video_file.ini"s;
	EXPECT_TRUE(mrpt::system::fileExists(ini_fil));

	const std::string video_fil =
		mrpt::UNITTEST_BASEDIR + "/share/mrpt/datasets/dummy_video.avi"s;
	EXPECT_TRUE(mrpt::system::fileExists(video_fil));

	try
	{
		mrpt::apps::RawlogGrabberApp app;
		app.setMinLoggingLevel(mrpt::system::LVL_ERROR);

		const char* argv[] = {"rawlog-grabber", ini_fil.c_str()};
		const int argc = sizeof(argv) / sizeof(argv[0]);

		app.initialize(argc, argv);

		// Create output dir:
		const auto out_dir = mrpt::system::getTempFileName() + "_dir"s;
		if (!mrpt::system::createDirectory(out_dir))
			THROW_EXCEPTION_FMT("Could not mkdir: `%s`", out_dir.c_str());

		app.params.write("global", "rawlog_prefix", out_dir + "/dataset"s);
		app.params.write("Camera1", "ffmpeg_url", video_fil);

		// Run slowly, so we have time to capture a few frames from the
		// (otherwise really short) test video file:
		app.params.write("Camera1", "process_rate", "10.0");

		// Max. run time.
		// Should end much sooner when the video file is entirely processed.
		app.run_for_seconds = 45.0;

		// Less verbose output in tests:
		app.show_sensor_thread_exceptions = false;

		const std::size_t REQUIRED_GRAB_OBS = 1U;

		auto tWatchDog = std::thread([&]() {
			for (;;)
			{
				if (!app.isRunning()) break;
				if (app.rawlog_saved_objects >= REQUIRED_GRAB_OBS)
				{
					app.run_for_seconds = 1.0;  // make it exit
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
		});

		// Run:
		app.run();

		// Check expected results:
		std::cout << "Rawlog grabbed objects: " << app.rawlog_saved_objects
				  << std::endl;
		if (tWatchDog.joinable()) tWatchDog.join();

		EXPECT_GE(app.rawlog_saved_objects, REQUIRED_GRAB_OBS);
	}
	catch (const std::exception& e)
	{
		std::cerr << mrpt::exception_to_str(e);
		GTEST_FAIL();
	}
}
