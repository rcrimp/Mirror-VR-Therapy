#include <opencv2/core/core.hpp>

#define CURRENT 0
#define PREVIOUS 1
#define IMG_WIDTH 640
#define IMG_HEIGHT 240
enum image_type {LEFT_IMG, RIGHT_IMG, LEFT_DIFF, RIGHT_DIFF, THRESH, HIST1, HIST2};

struct frameInfoContainer {
	cv::Mat leftImage;
	cv::Mat rightImage;
	cv::Mat leftDifference;
	cv::Mat rightDifference;

	cv::Mat temp_threshold;
	cv::Mat temp_thresh_hist;
	cv::Mat temp_diff_hist;
};

class frameInfoStructure {
private:
	int current, size;
	long totalFrameCount;
	frameInfoContainer *buffer;
public:
	cv::Mat errorMat;

	// constructor
	frameInfoStructure(int s);

	// public methods
	int getFrameCount();
	int getSize();
	void addFrame(cv::Mat left, cv::Mat right);

	cv::Mat getImage(int history_steps, image_type type);
	/* 	t = {0, -1, ... , -(size-1)} = {top, next, ... , last} */
};