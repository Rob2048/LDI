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
// Window
//------------------------------------------------------------------------------------------------------------------------

//long __stdcall WindowProcedure(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
//{
//	switch (msg)
//	{
//	case WM_DESTROY:
//		std::cout << "\ndestroying window\n";
//		PostQuitMessage(0);
//		return 0L;
//	case WM_LBUTTONDOWN:
//		std::cout << "\nmouse left button down at (" << LOWORD(lp)
//			<< ',' << HIWORD(lp) << ")\n";
//		// fall thru
//	default:
//		std::cout << '.';
//		return DefWindowProc(window, msg, wp, lp);
//	}
//}



//------------------------------------------------------------------------------------------------------------------------
// Sockets interface.
//------------------------------------------------------------------------------------------------------------------------
#define BUFLEN (1024 * 1024 * 16)
uint8_t _recvBuf[BUFLEN];
int _recvLen = 0;

uint8_t _packetData[BUFLEN];
int _packetRecvState = 0;
int _packetPayloadLen = 0;
int _packetSize = 0;

int readData(SOCKET ClientSocket, uint8_t* Data, int Size) {
	int dataRecvd = 0;

	while (dataRecvd != Size) {
		int iResult = recv(ClientSocket, (char*)(Data) + dataRecvd, Size - dataRecvd, 0);
		_recvLen = iResult;
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

uint8_t frameData[1640 * 1232];

int handleCmdFrame(SOCKET ClientSocket) {
	std::cout << "Handle frame\n";

	int frameSize = 0;
	if (readData(ClientSocket, (uint8_t*)&frameSize, 4) != 0) return -1;

	int frameSizeLI = 0;

	int width = 1640;
	int height = 1232;

	if (readData(ClientSocket, frameData, width * height) != 0) return -1;

	std::cout << "Got frame " << frameSize << "\n";

	Mat image(Size(1664, 1200), CV_8UC1, frameData);

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

	// NOTE: This is absolute insanity. I hate this SOO much -_-.
	imshow("Beam profiler", imageDebug);
	waitKey(1);

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

//int runWindow() {
//	std::cout << "hello world!\n";
//	const char* const myclass = "myclass";
//	WNDCLASSEX wndclass = { sizeof(WNDCLASSEX), CS_DBLCLKS, WindowProcedure,
//	0, 0, GetModuleHandle(0), LoadIcon(0,IDI_APPLICATION),
//	LoadCursor(0,IDC_ARROW), HBRUSH(COLOR_WINDOW + 1),
//	0, myclass, LoadIcon(0,IDI_APPLICATION) };
//	if (RegisterClassEx(&wndclass))
//	{
//		HWND window = CreateWindowEx(0, myclass, "title",
//			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
//			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);
//		if (window)
//		{
//			ShowWindow(window, SW_SHOWDEFAULT);
//			MSG msg;
//			while (GetMessage(&msg, 0, 0, 0)) DispatchMessage(&msg);
//		}
//	}
//}

int main() {
    std::cout << "Beam profiler\n";

    runServer();
	
    std::cout << "Beam profiler complete\n";

    return 0;
}

