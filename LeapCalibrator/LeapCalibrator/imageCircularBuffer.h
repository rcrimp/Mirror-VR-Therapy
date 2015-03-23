#include <opencv2/core/core.hpp>

#define CURRENT 0
#define PREVIOUS -1
enum image_type {LEFT, RIGHT, DIFF};

struct imageContainer {
	cv::Mat left;
	cv::Mat right;
	cv::Mat difference;
};

class imageCircularBuffer {
private:
	int current, count, size;
	imageContainer *buffer;
public:

	// constructor
	imageCircularBuffer(int s);

	// public methods
	int getCount();
	int getSize();
	void add(cv::Mat left, cv::Mat right);

	cv::Mat get(image_type type, int t);
	/* 	t = {0, -1, ... , -(size-1)} = {top, next, ... , last} */
};