#include "camera.h"

#include "LibCamera.h"
#include <string.h>

struct ldiCamContext {
	int width;
	int height;
	LibCamera cam;
	ControlList controls;
	LibcameraOutData frameData;
};

ldiCamContext* cameraCreateContext() {
	return new ldiCamContext;
}

int cameraInit(ldiCamContext* Context) {
	Context->cam.initCamera();

	return 0;
}

int cameraStart(ldiCamContext* Context, int Width, int Height, int Fps, int ShutterSpeed, int Gain, int* FrameSize, int* FrameStride) {
	std::cout << "Starting camera\n";

	Context->width = Width;
	Context->height = Height;

	// formats::RGB888
	int ret = Context->cam.startCamera(Width, Height, formats::YUV420, 4, 0, FrameSize, FrameStride);

	if (ret) {
		std::cout << "Cam init failure\n";
		Context->cam.closeCamera();
		return 1;
	}

	/*
	Capture from camera:

	Control: 7 ExposureTime: 396
	Control: 20 FocusFoM: 271
	Control: 8 AnalogueGain: 1.000000
	Control: 21 ColourCorrectionMatrix: [ 1.828732, -0.745506, -0.083221, -0.336660, 1.921851, -0.585198, -0.094487, -0.594186, 1.688676 ]
	Control: 24 FrameDuration: 99999
	Control: 11 Lux: 1527.501465
	Control: 2 AeLocked: true
	Control: 15 ColourGains: [ 2.057137, 1.504434 ]
	Control: 23 DigitalGain: 1.007623
	Control: 16 ColourTemperature: 6492
	Control: 18 SensorBlackLevels: [ 4096, 4096, 4096, 4096 ]
	Control: 22 ScalerCrop: (680, 692)/1920x1080
	Control: 27 SensorTimestamp: 7617694874000
	*/

	// NOTE: https://github.com/raspberrypi/libcamera-apps/blob/main/core/libcamera_app.cpp
	int64_t frame_time = 1000000 / Fps;
	// 	int64_t minFrameTime = 1000000 / 30;
	Context->controls.set(controls::FrameDurationLimits, { frame_time, frame_time });
	Context->controls.set(controls::ExposureTime, ShutterSpeed);
	// NOTE: Analog gain at 0 is some kind of automatic?
	Context->controls.set(controls::AnalogueGain, Gain);
	// Context->controls.set(controls::DigitalGain, 1); NOTE: Not handled by driver.
	// Context->controls.set(controls::draft::NoiseReductionMode, controls::draft::NoiseReductionModeOff);
	Context->controls.set(controls::Sharpness, 0);
	Context->controls.set(controls::AwbMode, libcamera::controls::AwbCustom);
	Context->controls.set(controls::ColourGains, { 1.0f, 1.0f });

	// controls_.set(controls::ScalerCrop, crop);

	// 	controls_.set(controls::DigitalGain, 1);

	Context->cam.set(Context->controls);
	Context->cam.startCapture();

	return 0;
}

int cameraStop(ldiCamContext* Context) {
	Context->cam.stopCamera();
	Context->cam.closeCamera();

	return 0;
}

int cameraSetControls(ldiCamContext* Context, int Exposure, int Gain) {
	Context->controls.set(controls::ExposureTime, Exposure);
	Context->controls.set(controls::AnalogueGain, Gain);
	Context->cam.set(Context->controls);

	return 0;
}

int cameraSetFps(ldiCamContext* Context, int Fps) {
	int64_t frameTime = 1000000 / Fps;
	Context->controls.set(controls::FrameDurationLimits, { frameTime, frameTime });
	Context->cam.set(Context->controls);

	return 0;
}

bool cameraReadFrame(ldiCamContext* Context) {
	return Context->cam.readFrame(&Context->frameData);
}

// int64_t cameraGetFrameTimestamp(ldiCamContext* Context) {
// 	return Context->frameData.timestamp;
// }

// int cameraGetFrameDataSize(ldiCamContext* Context) {
// 	return Context->frameData.size;
// }

LibcameraOutData* cameraGetFrameData(ldiCamContext* Context) {
	return &Context->frameData;
}

void cameraCopyFrame(ldiCamContext* Context, uint8_t* Buffer) {
	memcpy(Buffer, Context->frameData.imageData, Context->frameData.size);
}

int cameraReturnFrameBuffer(ldiCamContext* Context) {
	Context->cam.returnFrameBuffer(Context->frameData);

	return 0;
}