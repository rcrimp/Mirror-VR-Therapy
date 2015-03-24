#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <Leap.h>
#include <windows.h>
#include "frameInfoStructure.h"

using namespace cv;

#define RECORD_VIDEO 0
#define RECORD_ALL_IMAGES 0

// the amount of frames to store in the image history buffer
#define NUM_HISTORY_FRAMES 5 

int main(int argc, char** argv)
{
	Leap::Controller controller = Leap::Controller();
	controller.setPolicy(Leap::Controller::POLICY_IMAGES);
	controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

	/* setup the image window */
	cv::Mat windowImage = cv::Mat(cv::Size(IMG_WIDTH * 2, IMG_HEIGHT * 3), CV_8UC1); // default image
	windowImage.setTo(cv::Scalar(0, 0, 0));
	cv::namedWindow("Leap Motion images", cv::WINDOW_AUTOSIZE); // Create a window for display.

	while (!controller.isConnected()); // wait till the controller is connected

	/* setup image data structure */
	frameInfoStructure imageBuffer = frameInfoStructure(NUM_HISTORY_FRAMES);
	
	/* begin video recording */
	cv::VideoWriter outputVideo;
	if (RECORD_VIDEO)
		outputVideo.open("images/test.avi", CV_FOURCC('D', 'I', 'V', 'X'), 30.0, cv::Size(IMG_WIDTH * 2, IMG_HEIGHT * 3), false);
	
	/* begin main loop */
	do {
		Leap::Frame frame;
		Leap::ImageList images;
		std::ostringstream ss;

		/* poll for valid frame and non-empty imageList */
		do {
			frame = controller.frame();
			if (frame.isValid())
				images = frame.images();
		} while (images.isEmpty());

		/* add the new Leap images to the imagebuffer structure thing */
		// assert images.count = 2
		// assert both image width() and height() = IMG_WIDTH and IMG_HEIGHT respectively
		imageBuffer.addFrame(
			cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1, (void*)images[0].data()), // left camera image
			cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1, (void*)images[1].data())); // right camera image

		/* copy the latest Leap images to the window buffer */
		imageBuffer.getImage(0, LEFT_IMG).copyTo(windowImage(cv::Rect(0, 0, IMG_WIDTH, IMG_HEIGHT)));
		imageBuffer.getImage(0, RIGHT_IMG).copyTo(windowImage(cv::Rect(IMG_WIDTH, 0, IMG_WIDTH, IMG_HEIGHT)));
		/* previous images */
		imageBuffer.getImage(1, LEFT_IMG).copyTo(windowImage(cv::Rect(0, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
		imageBuffer.getImage(1, RIGHT_IMG).copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
		/* latest difference images */
		imageBuffer.getImage(0, LEFT_DIFF).copyTo(windowImage(cv::Rect(0, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
		imageBuffer.getImage(0, RIGHT_DIFF).copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
		

		/* slap on all the overlay text */
		ss.str("");
		ss << frame.currentFramesPerSecond() << " FPS"; // framerate
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		ss.str("");
		ss << "frame " << imageBuffer.getFrameCount(); // current frame count
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 40), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "Left", cv::Point2d(320, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "Right", cv::Point2d(980, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "current", cv::Point(20, 120), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "previous", cv::Point(20, 360), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "difference", cv::Point(20, 600), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		
		// show the image
		cv::imshow("Leap Motion images", windowImage);

		/* save the image as file */
		if (RECORD_ALL_IMAGES){
			ss.str("");
			ss << "images/Leap_" << imageBuffer.getFrameCount() << ".png";
			cv::imwrite(ss.str(), windowImage);
		}
		/* append the frame to the video */
		if (RECORD_VIDEO) {
			outputVideo << windowImage;
		}
	} while (cv::waitKey(1) < 0);
	return 0;
}