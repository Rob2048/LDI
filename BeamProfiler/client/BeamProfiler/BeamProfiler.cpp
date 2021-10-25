#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

//#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>

#pragma comment (lib, "Ws2_32.lib")

using namespace cv;

#include <iostream>

//------------------------------------------------------------------------------------------------------------------------
// Sockets interface.
//------------------------------------------------------------------------------------------------------------------------
#define BUFLEN (1024 * 1024 * 16)
uint8_t _recvBuf[BUFLEN];

uint8_t _packetData[BUFLEN];
int _packetRecvState = 0;
int _packetPayloadLen = 0;
int _packetSize = 0;

int readData(SOCKET ClientSocket, uint8_t* Data, int Size) {
	int dataRecvd = 0;

	while (dataRecvd != Size) {
		int iResult = recv(ClientSocket, (char*)(Data) + dataRecvd, Size - dataRecvd, 0);
		// std::cout << "Got " << dataRecvd << "/" << Size << " " << iResult << "\n";

		if (iResult > 0) {
			dataRecvd += iResult;
		}
		else if (iResult == 0) {
			std::cout << "Client connection closing\n" << std::flush;
			return -1;
		}
		else {
			std::cout << "Client connection error: " << WSAGetLastError() << "\n" << std::flush;
			return -1;
		}
	}

	return 0;
}

HANDLE _serverThread;
volatile int32_t _frameDataLock = 0;
uint8_t _processFrameData[1640 * 1232];
uint8_t _sharedFrameData[1640 * 1232];
uint8_t _netFrameData[1640 * 1232];
volatile int32_t _sharedFrameNew = 1;

void printOpenCVMatrixType(Mat mat) {
	std::string r;

	int type = mat.type();

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	printf("Matrix: %s %dx%d \n", r.c_str(), mat.cols, mat.rows);
}

int handleCmdFrame(SOCKET ClientSocket) {
	std::cout << "Handle frame\n";

	int frameSize = 0;
	if (readData(ClientSocket, (uint8_t*)&frameSize, 4) != 0) return -1;

	int frameSizeLI = 0;

	int width = 1640;
	int height = 1232;

	if (readData(ClientSocket, _netFrameData, width * height) != 0) return -1;

	// If we have frame, the send it to shared buffer.
	while (InterlockedCompareExchange((volatile long*)&_frameDataLock, 1, 0) == 1);
	memcpy(_sharedFrameData, _netFrameData, sizeof(_sharedFrameData));
	_sharedFrameNew = 1;
	InterlockedDecrement((volatile long*)&_frameDataLock);

	std::cout << "Got frame on net layer" << frameSize << "\n";
	
	return 0;
}

bool runServer() {
	WSADATA wsaData;
	int iResult;

	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET clientSocket = INVALID_SOCKET;

	int iSendResult;

	int recvBufLen = BUFLEN;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cout << "Net startup failed: " << iResult << "\n";
		return false;
	}

	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, "50001", &hints, &result);
	if (iResult != 0) {
		std::cout << "Getaddrinfo failed: " << iResult << "\n";
		WSACleanup();
		return false;
	}

	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "Listen socket failed: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Bind failed: " << WSAGetLastError() << "\n";
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Listen failed: " << WSAGetLastError() << "\n";
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	std::cout << "Net started successfully\n" << std::flush;

	while (true) {
		// Accept
		clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			std::cout << "Accept failed: " << WSAGetLastError() << "\n";
			closesocket(listenSocket);
			WSACleanup();
			return false;
		}

		std::cout << "Got client connection " << (int)clientSocket << "\n" << std::flush;

		// Receive until shutdown.
		while (true) {
			int32_t cmd = 0;
			if (readData(clientSocket, (uint8_t*)&cmd, 4) != 0) break;

			if (cmd == 1) {
				if (handleCmdFrame(clientSocket) != 0) break;
			}
		}	

		closesocket(clientSocket);
	}

	closesocket(listenSocket);
	WSACleanup();

	return true;
}

void CreateConsole() {
	AllocConsole();

	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}

HDC hDibDC;

LRESULT CALLBACK WndProc(HWND HWnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	switch (Msg) {	
		case WM_DESTROY: {
			PostQuitMessage(0);
		} break;
		
		case WM_PAINT: {
			std::cout << "Paint\n";
			PAINTSTRUCT paint;
			HDC hWndDC = BeginPaint(HWnd, &paint);
			BitBlt(hWndDC, 0, 0, 1280, 720, hDibDC, 0, 0, SRCCOPY);
			EndPaint(HWnd, &paint);
		} break;

		default: {
			return DefWindowProc(HWnd, Msg, WParam, LParam);
		}
	}

	return 0;
}

HWND InitializeWindow(HINSTANCE HInstance, int ShowWnd, int Width, int Height, bool Windowed) {
	RECT wr = { 0, 0, Width, Height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = HInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MainWindow";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		return 0;
	}

	HWND hwnd = CreateWindowEx(NULL, "MainWindow", "Beam profiler", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, HInstance, NULL);

	if (!hwnd) {
		return 0;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return hwnd;
}

DWORD WINAPI serverThreadProc(LPVOID lpParameter) {
	std::cout << "Starting server\n";
	runServer();
	//while (true) {
	//	// Do all socket net stuff here.
	//	Sleep(1000);
	//	std::cout << "Server tick\n";

	//	
	//}

	return 0;
}

int WINAPI WinMain(HINSTANCE HInstance, HINSTANCE HPrevInstance, LPSTR LPCmdLine, int NShowCmd) {
	CreateConsole();

	std::cout << "Starting profiler\n";

	int screenWidth = 1280;
	int screenHeight = 720;

	HWND windowHandle = InitializeWindow(HInstance, NShowCmd, screenWidth, screenHeight, true);

	HDC deviceContext = GetDC(windowHandle);

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = screenWidth;
	bmi.bmiHeader.biHeight = screenHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	uint8_t* backBuffer;

	HBITMAP hDib = CreateDIBSection(deviceContext, &bmi, DIB_RGB_COLORS, (void**)&backBuffer, 0, 0);
	if (backBuffer == NULL) {
		std::cout << "Could not create DIB\n";
		return 1;
	} else {
		std::cout << "Created backbuffer\n";
	}

	hDibDC = CreateCompatibleDC(deviceContext);
	HGDIOBJ hOldObj = SelectObject(hDibDC, hDib);

	for (int iY = 0; iY < screenHeight; ++iY) {
		for (int iX = 0; iX < screenWidth; ++iX) {
			int idx = iY * screenWidth + iX;

			backBuffer[idx * 4 + 0] = 128; // B
			backBuffer[idx * 4 + 1] = 128; // G
			backBuffer[idx * 4 + 2] = 128; // R
		}
	}

	RedrawWindow(windowHandle, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

	MSG msg = {};
	bool running = true;

	_serverThread = CreateThread(0, 0, serverThreadProc, NULL, 0, NULL);

	while (running) {
		
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				running = false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		
		bool updateRequired = false;

		// Check if there are any new frames to display.
		if (InterlockedCompareExchange((volatile long*)&_frameDataLock, 1, 0) == 1) {
			if (_sharedFrameNew == 1) {
				_sharedFrameNew = 0;
				updateRequired = true;
				memcpy(_processFrameData, _sharedFrameData, sizeof(_sharedFrameData));
			}

			InterlockedDecrement((volatile long*)&_frameDataLock);
		}

		if (updateRequired) {
			std::cout << "Got frame on process thread\n";
			Mat image(Size(1664, 1200), CV_8UC1, _sharedFrameData);

			Mat imageDebug;
			cv::cvtColor(image, imageDebug, cv::COLOR_GRAY2RGB);

			// Pink
			circle(imageDebug, Point2f(100, 100), 4, Scalar(182, 3, 252), FILLED);
			// Orange
			circle(imageDebug, Point2f(100, 120), 4, Scalar(3, 132, 252), FILLED);
			// Purple
			circle(imageDebug, Point2f(100, 140), 4, Scalar(252, 3, 182), FILLED);
			// Cyan
			circle(imageDebug, Point2f(100, 160), 4, Scalar(252, 132, 3), FILLED);

			putText(imageDebug, "Beam profiler", Point(10, 30), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(255, 255, 255), 1);

			// Copy to backbuffer.
			for (int iY = 0; iY < screenHeight; ++iY) {
				for (int iX = 0; iX < screenWidth; ++iX) {
					int idx = (screenHeight - 1 - iY) * screenWidth + iX;

					cv::Vec3b cols;

					if (iX < imageDebug.cols && iY < imageDebug.rows) {
						cols = imageDebug.at<cv::Vec3b>(iY, iX);
					}

					backBuffer[idx * 4 + 0] = cols[0]; // B
					backBuffer[idx * 4 + 1] = cols[1]; // G
					backBuffer[idx * 4 + 2] = cols[2]; // R
				}
			}

			printOpenCVMatrixType(imageDebug);

			RedrawWindow(windowHandle, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	return 0;
}

//int main() {
//    std::cout << "Beam profiler\n";
//
//    runServer();
//	
//    std::cout << "Beam profiler complete\n";
//
//    return 0;
//}
