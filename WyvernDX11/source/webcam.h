#pragma once

struct ldiWebcam {
	std::thread					workerThread;

	// Only worker thread.
	cv::VideoCapture			webcamCapture;

	// Shared by threads.
	std::mutex					frameMutex;
	volatile bool				frameAvailable = false;
	uint8_t* greyFrame;
	
	// Only main thread.
	// ...
};

void webcamWorkerThread(ldiWebcam* Webcam) {
	std::cout << "Running webcam worker thread\n";

	//----------------------------------------------------------------------------------------------------
	// Start webcamera camera.
	//----------------------------------------------------------------------------------------------------
	std::cout << "Start camera...\n";
	Webcam->webcamCapture.open(0, cv::CAP_ANY);

	if (!Webcam->webcamCapture.isOpened()) {
		std::cout << "Failed to open camera\n";
		return;
	} else {
		std::cout << "Camera started\n";
	}

	std::cout << "Webcam backend: " << Webcam->webcamCapture.getBackendName() << "\n";

	//Webcam->webcamCapture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	//Webcam->webcamCapture.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
	Webcam->webcamCapture.set(cv::CAP_PROP_EXPOSURE, -100);

	for (int i = 0; i < 71; ++i) {
		std::cout << i << ": " << Webcam->webcamCapture.get(i) << "\n";
	}

	cv::Mat webcamFrame;
	cv::Mat frameGrey;

	while (true) {
		if (Webcam->webcamCapture.read(webcamFrame)) {
			cv::cvtColor(webcamFrame, frameGrey, cv::COLOR_BGR2GRAY);

			std::unique_lock<std::mutex> lock(Webcam->frameMutex);
			Webcam->frameAvailable = true;
			memcpy(Webcam->greyFrame, frameGrey.data, 1280 * 720);

			lock.unlock();
		}
	}

	std::cout << "Webcam worker thread completed\n";
}

void webcamInit(ldiWebcam* Webcam) {
	Webcam->greyFrame = new uint8_t[1280 * 720];
	//Webcam->workerThread = std::thread(webcamWorkerThread, Webcam);
}

bool webcamProcess(ldiWebcam* Webcam) {
	std::unique_lock<std::mutex> lock(Webcam->frameMutex, std::defer_lock);
	if (!lock.try_lock()) {
		return false;
	}

	if (!Webcam->frameAvailable) {
		return false;
	}

	Webcam->frameAvailable = false;
	lock.unlock();

	return true;
}