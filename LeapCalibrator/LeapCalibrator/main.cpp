#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <Leap.h>
#include <windows.h>

using namespace cv;

inline int mod(int i, int n){
	return (i % n + n) % n;
}

struct imagePair{
	cv::Mat left;
	cv::Mat right;
};

class imageCircularBuffer {
private:
	int current;
	imagePair *buffer;

public:
	int count, size;

	imageCircularBuffer(int s = 3){
		size = s;
		current = 0;
		count = 0;
		buffer = new imagePair[size];
	}

	~imageCircularBuffer(){
		//delete[] buffer;
	}

	void add(imagePair images) {
		buffer[mod(++current, size)] = images;
		count = min(count + 1, size);
	}

	/* 	t = {0, -1, ... , -(size-1)} = {top, next, ... , last} */
	imagePair get(int t = 0) {
		return buffer[mod(current + t, size)];
		std::cout << "current: " << current << " requested: " << mod(current + t, size) << std::endl;
	}
};

#define IMG_WIDTH 640
#define IMG_HEIGHT 240

int main(int argc, char** argv)
{
	Leap::Controller controller;
	controller = Leap::Controller();
	controller.setPolicy(Leap::Controller::POLICY_IMAGES);
	controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

	/* setup the image window */
	cv::Mat windowImage = cv::Mat(480, 640, CV_8UC1); // default image
	windowImage.setTo(cv::Scalar(0, 0, 0));
	cv::namedWindow("Leap Motion images", cv::WINDOW_AUTOSIZE); // Create a window for display.
	cv::imshow("Leap Motion images", windowImage);

	while (!controller.isConnected()); // wait till the controller is connected
	std::cerr << "controller is connected" << std::endl;

	//cv::VideoWriter outputVideo;
	//outputVideo.open("test.avi", CV_FOURCC('D', 'I', 'V', 'X'), 30.0, cv::Size(IMG_WIDTH * 2, IMG_HEIGHT * 3), false);
	int frameCount = 0;
	imageCircularBuffer imageBuffer = imageCircularBuffer(3);
	/* begin main loop */
	do {
		Leap::Frame frame;
		Leap::ImageList images;
		std::ostringstream ss;

		// poll for valid frame and the imageList
		do {
			frame = controller.frame();
			if (frame.isValid())
				images = frame.images();
		} while (images.isEmpty());
		frameCount++;

		/* convert leap images to cv images */
		// assert images.count = 2
		// assert both image dimensions = 640 x 240 IMG_WIDHT x IMG_HEIGHT
		imagePair imp = {
			cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1, (void*)images[0].data()), // left camera image
			cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1, (void*)images[1].data()) // right camera image
		};
		imageBuffer.add(imp);

		cv::Mat diffL(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1);
		cv::Mat diffR(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1);
		diffL.setTo(cv::Scalar(0, 0, 0));
		diffR.setTo(cv::Scalar(0, 0, 0));
		if (imageBuffer.count > 2) {
			cv::add(imageBuffer.get(0).left, imageBuffer.get(-1).left, diffL);
			cv::absdiff(imageBuffer.get(0).right, imageBuffer.get(-1).right, diffR);
		}

		/* copy the leap images to the window buffer */
		windowImage = cv::Mat(cv::Size(IMG_WIDTH * 2, IMG_HEIGHT * 3), CV_8UC1);
		imageBuffer.get(0).left.copyTo(windowImage(cv::Rect(0, 0, IMG_WIDTH, IMG_HEIGHT)));
		imageBuffer.get(0).right.copyTo(windowImage(cv::Rect(IMG_WIDTH, 0, IMG_WIDTH, IMG_HEIGHT)));

		if (imageBuffer.count > 2) {
			imageBuffer.get(-1).left.copyTo(windowImage(cv::Rect(0, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
			imageBuffer.get(-1).right.copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));

		}
		diffL.copyTo(windowImage(cv::Rect(0, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));
		diffR.copyTo(windowImage(cv::Rect(IMG_WIDTH, IMG_HEIGHT * 2, IMG_WIDTH, IMG_HEIGHT)));

		// add the frame rate and frame cound to the image
		ss << frame.currentFramesPerSecond() << " FPS";
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		ss.str("");
		ss << "frame " << frameCount;
		cv::putText(windowImage, ss.str(), cv::Point2d(10, 40), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		ss.str("");

		cv::putText(windowImage, "Left", cv::Point2d(320, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "Right", cv::Point2d(980, 20), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "current", cv::Point(20, 120), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "previous", cv::Point(20, 360), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		cv::putText(windowImage, "difference", cv::Point(20, 600), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255, 255, 255), 1, CV_AA);
		//outputVideo << windowImage;
		//write the image to file
		//ss << "images/Leap_" << frameCount << ".png";
		//cv::imwrite(ss.str(), windowImage);

		// show the image
		cv::imshow("Leap Motion images", windowImage);

	} while (cv::waitKey(1) < 0);
	return 0;
}