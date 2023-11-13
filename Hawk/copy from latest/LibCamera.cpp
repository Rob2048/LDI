#include "LibCamera.h"

void _printConfigInfo(CameraConfiguration* config) {
	std::cout << "Streams: " << config->size() << "\n";

	for (int i = 0; i < config->size(); ++i) {
		std::cout << "Stream " << i << ":\n";
		std::cout << "  pixelFormat: " << config->at(i).pixelFormat << "\n";
		std::cout << "  size: " << config->at(i).size.width << ", " << config->at(i).size.height << "\n";
		std::cout << "  frameSize: " << config->at(i).frameSize << "\n";
		std::cout << "  bufferCount: " << config->at(i).bufferCount << "\n";
		std::cout << "  stride: " << config->at(i).stride << "\n";
		// std::cout << "  colorSpace: " << config->at(i).colorSpace << "\n";
	}
}

int LibCamera::initCamera() {
	cm = std::make_unique<CameraManager>();
	int ret = cm->start();

	if (ret) {
		std::cout << "Failed to start camera manager: " << ret << std::endl;
		return ret;
	}

	std::string cameraId = cm->cameras()[0]->id();
	camera_ = cm->get(cameraId);
	if (!camera_) {
		std::cerr << "Camera " << cameraId << " not found" << std::endl;
		return 1;
	}

	if (camera_->acquire()) {
		std::cerr << "Failed to acquire camera " << cameraId << std::endl;
		return 1;
	}
	camera_acquired_ = true;

	allocator_ = std::make_unique<FrameBufferAllocator>(camera_);
	
	return 0;
}

int LibCamera::startCamera(int width, int height, PixelFormat format, int buffercount, int rotation, int* FrameSize, int* FrameStride) {
	std::unique_ptr<CameraConfiguration> config;
	// StreamRole::Raw
	// StreamRole::VideoRecording
	// StreamRole::Viewfinder
	config = camera_->generateConfiguration({ StreamRole::Raw });
	
	libcamera::Size size(width, height);
	config->at(0).pixelFormat = format;
	config->at(0).size = size;
	config->at(0).colorSpace = ColorSpace::Raw;
	
	if (buffercount) {
		config->at(0).bufferCount = buffercount;
	}

	// Transform transform = Transform::Identity;
	// bool ok;
	// Transform rot = transformFromRotation(180, &ok);
	// if (!ok)
	// 	throw std::runtime_error("illegal rotation value, Please use 0 or 180");
	// transform = rot * transform;
	// if (!!(transform & Transform::Transpose))
	// 	throw std::runtime_error("transforms requiring transpose not supported");
	// config->transform = transform;

	std::cout << "Before validation:\n";
	_printConfigInfo(config.get());

	switch (config->validate()) {
		case CameraConfiguration::Valid:
			break;

		case CameraConfiguration::Adjusted:
			std::cout << "Camera configuration adjusted" << std::endl;
			break;

		case CameraConfiguration::Invalid:
			std::cout << "Camera configuration invalid" << std::endl;
			return 1;
	}

	std::cout << "Final configuration:\n";
	_printConfigInfo(config.get());

	*FrameSize = config->at(0).frameSize;
	*FrameStride = config->at(0).stride;

	config_ = std::move(config);

	int ret = camera_->configure(config_.get());
	if (ret < 0) {
		std::cout << "Failed to configure camera" << std::endl;
		return ret;
	}

	camera_->requestCompleted.connect(this, &LibCamera::requestComplete);

	unsigned int nbuffers = UINT_MAX;
	for (StreamConfiguration &cfg : *config_) {
		ret = allocator_->allocate(cfg.stream());
		if (ret < 0) {
			std::cerr << "Can't allocate buffers" << std::endl;
			return -ENOMEM;
		}

		unsigned int allocated = allocator_->buffers(cfg.stream()).size();
		nbuffers = std::min(nbuffers, allocated);

		std::cout << "Allocate buffer in stream conf: " << allocated << " " << nbuffers << "\n";
	}

	for (unsigned int i = 0; i < nbuffers; i++) {
		std::cout << "Buffer " << i << "\n";
		std::unique_ptr<Request> request = camera_->createRequest();
		if (!request) {
			std::cerr << "Can't create request" << std::endl;
			return -ENOMEM;
		}

		for (StreamConfiguration &cfg : *config_) {
			std::cout << "  Iter cfg\n";
			Stream *stream = cfg.stream();
			const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
				allocator_->buffers(stream);
			const std::unique_ptr<FrameBuffer> &buffer = buffers[i];

			ret = request->addBuffer(stream, buffer.get());
			if (ret < 0) {
				std::cerr << "Can't set buffer for request"
					  << std::endl;
				return ret;
			}

			int doneplane1 = 0;
			for (const FrameBuffer::Plane &plane : buffer->planes()) {
				std::cout << "    FrameBuffer plane length: " << plane.length << "\n";
				
				void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset);
				std::cout << "MEM: " << (uint64_t)memory << "\n";
				std::cout << "Plane FD: " << plane.fd.get() << "\n";

				if (!doneplane1) {
					mappedBuffers_[plane.fd.get()] = std::make_pair(memory, plane.length);
					doneplane1 = 1;
				}
			}
		}

		requests_.push_back(std::move(request));
	}

	return 0;
}

int LibCamera::startCapture() {
	int ret = camera_->start(&controls_);
	// ret = camera_->start();

	if (ret) {
		std::cout << "Failed to start capture" << std::endl;
		return ret;
	}

	controls_.clear();	
	camera_started_ = true;

	for (std::unique_ptr<Request> &request : requests_) {
		ret = queueRequest(request.get());
		if (ret < 0) {
			std::cerr << "Can't queue request" << std::endl;
			camera_->stop();
			return ret;
		}
	}

	return 0;
}

int LibCamera::queueRequest(Request *request) {
	std::lock_guard<std::mutex> stop_lock(camera_stop_mutex_);
	
	if (!camera_started_) {
		return -1;
	}

	{
		std::lock_guard<std::mutex> lock(control_mutex_);
		request->controls() = std::move(controls_);
	}

	return camera_->queueRequest(request);
}

void LibCamera::requestComplete(Request *request) {
	if (request->status() == Request::RequestCancelled)
		return;

	timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	int64_t systemTimeNs = (int64_t)time.tv_sec * 1000000000 + (int64_t)time.tv_nsec;

	std::lock_guard<std::mutex> lock(free_requests_mutex_);
	requestQueue.push(request);
}

void LibCamera::returnFrameBuffer(LibcameraOutData frameData) {
	uint64_t request = frameData.request;
	Request * req = (Request *)request;
	req->reuse(Request::ReuseBuffers);
	queueRequest(req);
}

bool LibCamera::readFrame(LibcameraOutData *frameData){
	std::lock_guard<std::mutex> lock(free_requests_mutex_);
	if (!requestQueue.empty()){
		Request *request = this->requestQueue.front();

		ControlList& metaData = request->metadata();

		// auto ts = metaData.get(controls::SensorTimestamp);
		// uint64_t timestamp = *ts;
		frameData->timestamp = *metaData.get(controls::SensorTimestamp);
		frameData->exposureTime = *metaData.get(controls::ExposureTime);
		frameData->analogGain = *metaData.get(controls::AnalogueGain);
		frameData->lux = *metaData.get(controls::Lux);
	
		// NOTE: Print list of controls on this frame.
		// for (auto it = metaData.begin(); it != metaData.end(); ++it) {
		// 	auto ctrlid = metaData.idMap()->at(it->first);
		// 	// std::cout << ctrlid->name() << "(" << el.first << ") = " << el.second.toString() << std::endl;
		// 	std::cout << " Control: " << it->first << " " << ctrlid->name() << ": " << it->second.toString() << "\n";
		// }

		const Request::BufferMap &buffers = request->buffers();
		// std::cout << "rf\n";
		for (auto it = buffers.begin(); it != buffers.end(); ++it) {
			// std::cout << "  buff\n";
			FrameBuffer *buffer = it->second;
			for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
				const FrameBuffer::Plane &plane = buffer->planes()[i];
				const FrameMetadata::Plane &meta = buffer->metadata().planes()[i];

				int planeFd = plane.fd.get();
				void *data = (uint8_t*)mappedBuffers_[planeFd].first + plane.offset;
				unsigned int length = std::min(meta.bytesused, plane.length);

				// std::cout << "    plane " << planeFd << " " << plane.offset << " " << length << " " << meta.bytesused << "\n";

				frameData->size = length;
				frameData->imageData = (uint8_t *)data;

				break;
			}
		}
		
		this->requestQueue.pop();
		frameData->request = (uint64_t)request;
		return true;
	} else {
		// TODO: Why is this needed?
		frameData->request = (uint64_t)nullptr;
		return false;
	}
}

void LibCamera::set(ControlList controls){
	std::lock_guard<std::mutex> lock(control_mutex_);
	controls_ = std::move(controls);
}

void LibCamera::stopCamera() {
	if (camera_) {
		{
			std::lock_guard<std::mutex> lock(camera_stop_mutex_);
			
			if (camera_started_) {
				if (camera_->stop()) {
					throw std::runtime_error("failed to stop camera");
				}

				camera_started_ = false;
			}
		}

		if (camera_started_) {
			throw std::runtime_error("failed to stop camera");
		}

		camera_->requestCompleted.disconnect(this, &LibCamera::requestComplete);
	
		while (!requestQueue.empty()) {
			requestQueue.pop();
		}

		requests_.clear();

		for (StreamConfiguration &cfg : *config_) {
			std::cout << "Free stream...\n";
			if (allocator_->free(cfg.stream()) != 0) {
				std::cout << "Failed to free stream buffers." << std::endl;
			}
		}

		if (allocator_->allocated()) {
			std::cout << "Buffers still allocated...\n";
		}

		for (auto it = mappedBuffers_.begin(); it != mappedBuffers_.end(); ++it) {
			std::cout << "Unmap buffer...\n";
			munmap(it->second.first, it->second.second);
		}

		mappedBuffers_.clear();

		allocator_.reset();
		controls_.clear();
	}
}

void LibCamera::closeCamera(){
	if (camera_acquired_) {
		camera_->release();
		camera_acquired_ = false;
	}

	camera_.reset();

	if (cm) {
		cm->stop();
		cm.reset();
	}

	config_.reset();

	sleep(1);
}
