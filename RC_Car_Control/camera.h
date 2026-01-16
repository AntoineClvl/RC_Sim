#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <atomic>

int openCamera();
void cameraThreadFunc();