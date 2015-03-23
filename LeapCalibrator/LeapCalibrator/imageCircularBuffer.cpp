#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "imageCircularBuffer.h"
#include <iostream>

inline int min(int a, int b){
	return ((a) > (b)) ? a : b;
}

inline int mod(int i, int n){
	return (i % n + n) % n;
}

imageCircularBuffer::imageCircularBuffer(int s){
	size = s;
	current = 0;
	count = 0;
	buffer = new imageContainer[size];
}

int imageCircularBuffer::getCount(){ return count; }
int imageCircularBuffer::getSize(){ return size; }

void imageCircularBuffer::add(cv::Mat left, cv::Mat right){
	imageContainer container;
	cv::Mat diff;
	
	diff = (cv::Size(left.cols, left.rows), CV_8UC1);
	diff.setTo(cv::Scalar(0, 0, 0));
	
	container = { left, right, diff };
	
	buffer[mod(++current, size)] = container;
	count = min(count + 1, size);
}

/* 	t = {0, -1, ... , -(size-1)} = {top, next, ... , last} */
cv::Mat imageCircularBuffer::get(image_type type, int t){
	imageContainer target = buffer[mod(current + t, size)];
	switch (type) {
	case LEFT:
		return target.left;
	case RIGHT:
		return target.right;
	case DIFF :
		return target.difference;
	default:
		return cv::Mat();
	}
}
