#include <opencv/cv.h>
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

cv::Mat absDiffEx(cv::Mat a, cv::Mat b){
	cv::Mat diff = cv::Mat(cv::Size(a.cols, a.rows), CV_8UC1);
	cv::absdiff(a, b, diff);
	return diff;
}

cv::Mat thresholdEx(cv::Mat a){
	cv::Mat thresh = cv::Mat(cv::Size(a.cols, a.rows), CV_8UC1);
	cv::threshold(a, thresh, 40, 255, cv::THRESH_BINARY);
	return thresh;
}

cv::Mat calculateHist(const cv::Mat img, int size) {
	int histSize = size;
	float range[] = { 0, size };
	const float* histRange = { range };

	bool uniform = true;
	bool accumulate = false;

	cv::Mat hist;
	const int *channels = new int[]{ 1 };
	cv::calcHist(&img, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

	/* histogram to image */
	int hist_w = 640; int hist_h = 240;
	int bin_w = cvRound((double)hist_w / histSize);
	cv::Mat histImage(hist_h, hist_w, CV_8UC1, cv::Scalar(0, 0, 0));

	float max = 0.0f;
	for (int i = 0; i < histSize; i++){
		max = (max > hist.at<float>(i) ? max : hist.at<float>(i));
	}
	max /= 10;

	for (int i = 0; i < histSize; i++){
		cv::line(histImage,
			cv::Point((bin_w * i), 239 - hist_h*(hist.at<float>(i) / max)),
			cv::Point((bin_w * i), 239),
			cv::Scalar(255, 0, 0),
			1, 8, 0);
	}

	/*normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());
	for (int i = 1; i < histSize; i++)
	{
	cv::line(histImage,
	cv::Point(bin_w*(i - 1), (hist_h - cvRound(hist.at<float>(i - 1)) )),
	cv::Point(bin_w*(i), (hist_h - cvRound(hist.at<float>(i)))),
	cv::Scalar(255, 0, 0), 2, 8, 0);
	}*/

	return histImage;
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
		container.leftDifference = absDiffEx(leftImage, getImage(0, LEFT_IMG));
		container.rightDifference = absDiffEx(rightImage, getImage(0, RIGHT_IMG));
		container.temp_threshold = thresholdEx(container.leftDifference);
		container.temp_diff_hist = calculateHist(container.leftDifference, 256);
		container.temp_thresh_hist = calculateHist(container.temp_threshold, 256);
		container.thresh_sum = (int)cv::sum(container.temp_thresh_hist).val[0] - 126225;
	} else {
		container.leftDifference = errorMat;
		container.rightDifference = errorMat;
		container.temp_threshold = errorMat;
		container.temp_diff_hist = errorMat;
		container.temp_thresh_hist = errorMat;
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
	case LEFT_IMG:	 return target.leftImage;
	case RIGHT_IMG:	 return target.rightImage;
	case LEFT_DIFF:	 return target.leftDifference;
	case RIGHT_DIFF: return target.rightDifference;
	case HIST1:	 return target.temp_diff_hist;
	case HIST2:	 return target.temp_thresh_hist;
	case THRESH: return target.temp_threshold;
	default: return errorMat;
	}
}

frameInfoContainer frameInfoStructure::getContainer(int history){
	if (history >= totalFrameCount || history >= size) {
		history = 0;
	}
	return buffer[mod(current - history, size)];
}