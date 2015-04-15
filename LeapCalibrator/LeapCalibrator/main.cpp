#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.h>
#include <iostream>
#include <Leap.h>
#include <windows.h>
#include "frameInfoStructure.h"

using namespace cv;

#define RECORD_VIDEO 0
#define RECORD_IMAGES 0

// the amount of frames to store in the image history buffer
#define NUM_HISTORY_FRAMES 5

int main(int argc, char** argv)
{
	Leap::Controller controller = Leap::Controller();
	controller.setPolicy(Leap::Controller::POLICY_IMAGES);
	controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

	/* setup the image window */
	cv::Mat windowImage = cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT * 3), CV_8UC3, cv::Scalar(0, 0, 0)); // default image
	windowImage.setTo(cv::Scalar(0, 0, 0));
	cv::namedWindow("Leap Motion images", cv::WINDOW_AUTOSIZE); // Create a window for display.
	cv::imshow("Leap Motion images", windowImage);

	/* setup image data structure */
	frameInfoStructure imageBuffer = frameInfoStructure(NUM_HISTORY_FRAMES);
	
	/* begin video recording */
	cv::VideoWriter outputVideo;
	if (RECORD_VIDEO)
		outputVideo.open("img/test.avi", CV_FOURCC('M', 'J', 'P', 'G'), 60.0, cv::Size(IMG_WIDTH, IMG_HEIGHT * 3), true);
	
	while (!controller.isConnected()); // wait till the controller is connected
	cv::Mat blobImage(IMG_HEIGHT, IMG_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
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

		cv::Mat col1, col2;
		cv::cvtColor(imageBuffer.getImage(0, LEFT_IMG), col1, CV_GRAY2RGB);
		cv::cvtColor(imageBuffer.getImage(0, LEFT_DIFF), col2, CV_GRAY2RGB);
		
		col1.copyTo(windowImage(cv::Rect(0, 0, IMG_WIDTH, IMG_HEIGHT)));
		col2.copyTo(windowImage(cv::Rect(0, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
		blobImage.copyTo(windowImage(cv::Rect(0, IMG_HEIGHT*2, IMG_WIDTH, IMG_HEIGHT)));
		
		/* copy the latest Leap images to the window buffer */
		//imageBuffer.getImage(0, LEFT_IMG).copyTo(windowImage(cv::Rect(0, 0, IMG_WIDTH, IMG_HEIGHT)));
		//imageBuffer.getImage(0, RIGHT_IMG).copyTo(windowImage(cv::Rect(IMG_WIDTH, 0, IMG_WIDTH, IMG_HEIGHT)));
		/* previous images */
		//imageBuffer.getImage(0, LEFT_DIFF).copyTo(windowImage(cv::Rect(0, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
		//imageBuffer.getImage(0, THRESH).copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
		/* latest difference images */
		//imageBuffer.getImage(0, HIST1).copyTo(windowImage(cv::Rect(0, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
		//imageBuffer.getImage(0, HIST2).copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
		

		/* slap on all the overlay text */
		ss.str("");
		ss << frame.currentFramesPerSecond() << " FPS"; // framerate
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		ss.str("");
		ss << "frame " << imageBuffer.getFrameCount(); // current frame count
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 40), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);

		if (imageBuffer.getContainer(0).thresh_sum == 0 &&
			imageBuffer.getContainer(1).thresh_sum > 1500 &&
			imageBuffer.getContainer(2).thresh_sum > 1500 &&
			imageBuffer.getContainer(3).thresh_sum == 0){

			cv::SimpleBlobDetector::Params params;
			//params.thresholdStep;
			params.minThreshold = 0;
			params.maxThreshold = 128;
			//params.minRepeatability = 10;
			params.minDistBetweenBlobs = 10.0f;

			params.filterByColor = false;
			//params.blobColor = 255;
			
			params.filterByArea = true;
			params.minArea = 10.0f;
			params.maxArea = 50.0f;

			params.filterByCircularity = true;
			params.minCircularity = 0.3f;
			params.maxCircularity = 1.0f;

			params.filterByInertia = false;
			//params.minInertiaRatio;
			//params.maxInertiaRatio;

			params.filterByConvexity = false;
			params.minConvexity = 0.2f;
			params.maxConvexity = 1.0f;

			cvtColor(imageBuffer.getImage(2, LEFT_DIFF), blobImage, CV_GRAY2RGB);
			cv::SimpleBlobDetector blobdetector(params);
			vector<cv::KeyPoint> keypoints;
			blobdetector.detect(blobImage, keypoints);
			for (int i = 0; i < keypoints.size(); i++){
				cv::circle(blobImage, keypoints[i].pt, keypoints[i].size*2, cv::Scalar(0, 0, 255), 1);
			}
			std::ostringstream ss2;
			ss2 << "img/Leap_" << imageBuffer.getFrameCount() << ".png";
			cv::imwrite(ss2.str(), blobImage);
		}

		//ss.str("");
		//ss << imageBuffer.getval(0) << " " << imageBuffer.getval(1) << " " << imageBuffer.getval(2) << " " << imageBuffer.getval(3);
		//cv::putText(windowImage, ss.str(), cv::Point2d(10, 80), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);

		//cv::putText(windowImage, ss.str(), cv::Point2d(10, 40), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		/*cv::putText(windowImage, "Left", cv::Point2d(320, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "Right", cv::Point2d(980, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "current", cv::Point(20, 120), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "previous", cv::Point(20, 360), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "difference", cv::Point(20, 600), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		*/

		// show the image
		cv::imshow("Leap Motion images", windowImage);

		/* save the image as file */
		if (RECORD_IMAGES){
			ss.str("");
			ss << "img/Leap_" << imageBuffer.getFrameCount() << ".png";
			cv::imwrite(ss.str(), windowImage);
		}

		/* append the frame to the video */
		if (RECORD_VIDEO) {
			outputVideo << windowImage;
		}
	} while (cv::waitKey(1) < 0);
	return 0;
}