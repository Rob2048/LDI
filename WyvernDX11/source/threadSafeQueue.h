#pragma once

#include <queue>
#include <thread>
#include <condition_variable>

struct ldiThreadSafeQueue {
	std::queue<void*>			queue;

	std::mutex					mutex;
	std::mutex					waitForElementMutex;
	std::condition_variable		waitForElementCondVar;
};

void tsqPush(ldiThreadSafeQueue* Queue, void* Element) {
	std::unique_lock<std::mutex> lock(Queue->mutex);
	Queue->queue.push(Element);
	Queue->waitForElementCondVar.notify_all();
}

void* tsqPop(ldiThreadSafeQueue* Queue) {
	std::unique_lock<std::mutex> lock(Queue->mutex);

	if (!Queue->queue.empty()) {
		void* element = Queue->queue.front();
		Queue->queue.pop();

		return element;
	}

	return nullptr;
}

void* tsqWaitPop(ldiThreadSafeQueue* Queue) {
	// TODO: Allow waiting threads to exit.

	while (true) {
		void* element = tsqPop(Queue);

		if (element != nullptr) {
			return element;
		}

		std::unique_lock<std::mutex> lock(Queue->waitForElementMutex);
		Queue->waitForElementCondVar.wait(lock);
	}
}

void tsqTerminate(ldiThreadSafeQueue* Queue) {
	Queue->waitForElementCondVar.notify_all();
}