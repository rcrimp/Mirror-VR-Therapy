#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <Leap.h>
#include <windows.h>
#include "imageCircularBuffer.h"

using namespace cv;

#define IMG_WIDTH 640
#define IMG_HEIGHT 240

#define RECORD_VIDEO 0
#define RECORD_ALL_IMAGES 0

int main(int argc, char** argv)
{
	Leap::Controller controller;
	controller = Leap::Controller();
	controller.setPolicy(Leap::Controller::POLICY_IMAGES);
	controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
	cv::VideoWriter outputVideo;
	int frameCount = 0;

	/* setup the image window */
	cv::Mat windowImage = cv::Mat(cv::Size(IMG_WIDTH * 2, IMG_HEIGHT * 3), CV_8UC1); // default image
	windowImage.setTo(cv::Scalar(0, 0, 0));
	cv::namedWindow("Leap Motion images", cv::WINDOW_AUTOSIZE); // Create a window for display.

	while (!controller.isConnected()); // wait till the controller is connected

	/* video recording */
	if (RECORD_VIDEO)
		outputVideo.open("images/test.avi", CV_FOURCC('D', 'I', 'V', 'X'), 30.0, cv::Size(IMG_WIDTH * 2, IMG_HEIGHT * 3), false);
	
	imageCircularBuffer imageBuffer = imageCircularBuffer(3);
	cv::Mat differenceLeft, differenceRight;
	differenceLeft = cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1);
	differenceRight = cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1);
	differenceLeft.setTo(cv::Scalar(0, 0, 0));
	differenceRight.setTo(cv::Scalar(0, 0, 0));
	/* begin main loop */
	do {
		Leap::Frame frame;
		Leap::ImageList images;
		std::ostringstream ss;

		/* poll for valid frame and the imageList */
		do {
			frame = controller.frame();
			if (frame.isValid())
				images = frame.images();
		} while (images.isEmpty());
		frameCount++;

		/* convert leap images to cv images */
		// assert images.count = 2
		// assert both image width() and height() = IMG_WIDTH and IMG_HEIGHT respectively
		imageBuffer.add(
			cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1, (void*)images[0].data()), // left camera image
			cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1, (void*)images[1].data())); // right camera image

		/* copy the leap images to the window buffer */
		imageBuffer.get(LEFT, 0).copyTo(windowImage(cv::Rect(0, 0, IMG_WIDTH, IMG_HEIGHT)));
		imageBuffer.get(RIGHT, 0).copyTo(windowImage(cv::Rect(IMG_WIDTH, 0, IMG_WIDTH, IMG_HEIGHT)));

		/* calculate the difference frames */
		if (imageBuffer.getCount() > 3) {
			cv::absdiff(imageBuffer.get(LEFT, 0), imageBuffer.get(LEFT, -1), differenceLeft);
			cv::absdiff(imageBuffer.get(RIGHT, 0), imageBuffer.get(RIGHT, -1), differenceRight);
		}

		if (imageBuffer.getCount() > 3) {
			imageBuffer.get(LEFT, -1).copyTo(windowImage(cv::Rect(0, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
			imageBuffer.get(RIGHT, -1).copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));

			differenceLeft.copyTo(windowImage(cv::Rect(0, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
			differenceRight.copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
		}
		

		/* slap on all the shitty text */
		ss.str("");
		ss << frame.currentFramesPerSecond() << " FPS"; // framerate
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		ss.str("");
		ss << "frame " << frameCount; // current frame count
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
			ss << "images/Leap_" << frameCount << ".png";
			cv::imwrite(ss.str(), windowImage);
		}
		/* append the frame to the video */
		if (RECORD_VIDEO) {
			outputVideo << windowImage;
		}

	} while (cv::waitKey(1) < 0);
	return 0;
}