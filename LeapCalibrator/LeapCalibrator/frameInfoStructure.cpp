#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "frameInfoStructure.h"
#include <iostream>

inline int min(int a, int b){
	return ((a) > (b)) ? a : b;
}

inline int mod(int i, int n){
	return (i % n + n) % n;
}

frameInfoStructure::frameInfoStructure(int s){
	size = s;
	current = 0;
	totalFrameCount = 0;
	buffer = new frameInfoContainer[size];
	
	/* init the error mat */
	errorMat = cv::Mat(cv::Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC1);
	errorMat.setTo(cv::Scalar(0, 0, 0));
	cv::putText(errorMat, "ERROR", cv::Point(50, 170), cv::FONT_HERSHEY_PLAIN, 10, cv::Scalar(255, 0, 0), 10, CV_AA);
}

int frameInfoStructure::getFrameCount(){ return totalFrameCount; }
int frameInfoStructure::getSize(){ return size; }

void frameInfoStructure::addFrame(cv::Mat leftImage, cv::Mat rightImage){
	frameInfoContainer container;
	container.leftImage = leftImage;
	container.rightImage = rightImage;
	
	if (totalFrameCount > 1) { // if a previous frame exists calculate the diff
		cv::Mat leftDifference = cv::Mat(cv::Size(leftImage.cols, leftImage.rows), CV_8UC1);
		cv::Mat rightDifference = cv::Mat(cv::Size(rightImage.cols, rightImage.rows), CV_8UC1);
		cv::absdiff(leftImage, getImage(0, LEFT_IMG), leftDifference);
		cv::absdiff(rightImage, getImage(0, RIGHT_IMG), rightDifference);
		container.leftDifference = leftDifference;
		container.rightDifference = rightDifference;
	} else {
		container.leftDifference = errorMat;
		container.rightDifference = errorMat;
	}

	buffer[mod(++current, size)] = container;
	totalFrameCount++;
}

/* returns the requested frame */
/* history determines how far back in the past the returned frame is
 * history=0 is the latest, most current frame
 * history=1 is the previous frame
 * history=size-1 is the oldest frame available */
cv::Mat frameInfoStructure::getImage(int history, image_type type){
	// if the requsted frame hasn't been generated yet OR will never be generated then return an empty image
	if (history >= totalFrameCount || history >= size) {
		return errorMat;
	}

	frameInfoContainer target = buffer[mod(current - history, size)];
	switch (type) {
	case LEFT_IMG:
		return target.leftImage;
	case RIGHT_IMG:
		return target.rightImage;
	case LEFT_DIFF:
		return target.leftDifference;
	case RIGHT_DIFF:
		return target.rightDifference;
	default:
		return errorMat;
	}
}
