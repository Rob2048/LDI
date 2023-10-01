#pragma once

// https://learn.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)

// TODO: Check for available serial ports:
// https://github.com/serialport/bindings-cpp/blob/main/src/serialport_win.cpp

struct ldiSerialPort {
	HANDLE descriptor = INVALID_HANDLE_VALUE;
	bool connected = false;
};

bool serialPortDisconnect(ldiSerialPort* Port) {
	if (Port->descriptor != INVALID_HANDLE_VALUE) {
		CloseHandle(Port->descriptor);
	}

	Port->descriptor = INVALID_HANDLE_VALUE;
	Port->connected = false;

	return true;
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString() {
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}


bool serialPortConnect(ldiSerialPort* Port, const char* Name, int BitRate) {
	serialPortDisconnect(Port);
	std::cout << "Attempt com connection: " << Name << "\n";

	HANDLE handle = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if (handle == INVALID_HANDLE_VALUE) {
		std::cout << "Serial Connect: Failed to open.\n";
		std::cout << "Winerr: " << GetLastErrorAsString() << "\n";
		return false;
	}

	PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

	DCB comParams = {};
	comParams.DCBlength = sizeof(comParams);
	GetCommState(handle, &comParams);
	comParams.BaudRate = BitRate;
	comParams.ByteSize = 8;
	comParams.StopBits = ONESTOPBIT;
	comParams.Parity = NOPARITY;
	comParams.fDtrControl = DTR_CONTROL_ENABLE;
	comParams.fRtsControl = DTR_CONTROL_ENABLE;

	BOOL status = SetCommState(handle, &comParams);

	if (status == FALSE) {
		// NOTE: Some USB<->Serial drivers require the state to be set twice.
		status = SetCommState(handle, &comParams);

		if (status == FALSE) {
			serialPortDisconnect(Port);
			return false;
		}
	}

	COMMTIMEOUTS timeouts = {};
	GetCommTimeouts(handle, &timeouts);
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(handle, &timeouts);

	Port->descriptor = handle;
	Port->connected = true;
	std::cout << "Serial Connect: Connected to " << Name << "\n";

	return true;
}

int serialPortWriteData(ldiSerialPort* Port, uint8_t* Buffer, int32_t BufferSize) {
	if (!Port->connected) {
		std::cout << "Serial Write: Invalid Serial Port.\n";
		return -1;
	}

	OVERLAPPED overlapped = {};
	DWORD bytesWritten = 0;

	if (!WriteFile(Port->descriptor, Buffer, BufferSize, &bytesWritten, &overlapped)) {
		DWORD lastError = GetLastError();
		
		if (lastError != ERROR_IO_PENDING) {
			std::cout << "Serial Write: Write Failed: << " << lastError << "\n";
			//serialPortDisconnect(Port);
			return -1;
		} else {
			 //STATUS_PENDING
			
			if (!GetOverlappedResult(Port->descriptor, &overlapped, &bytesWritten, true)) {
				std::cout << "Serial Write: Waiting Error.\n";
				//serialPortDisconnect(Port);
				return -1;
			} else {
				if (bytesWritten != BufferSize) {
					// TODO: This can occur if some other IO operation on the same handle completes before this one. Need to unique hEvent.
					std::cout << "Serial Write: Failed to write all bytes: " << bytesWritten << "/" << BufferSize << "\n";
					//serialPortDisconnect(Port);
					return -1;
				}

				return bytesWritten;
			}
		}
	} else {
		if (bytesWritten != BufferSize) {
			std::cout << "Serial Write: Failed to write all bytes: " << bytesWritten << "/" << BufferSize << "\n";
			//serialPortDisconnect(Port);
			return -1;
		}

		return bytesWritten;
	}

	return -1;
}

int32_t serialPortReadData(ldiSerialPort* Port, uint8_t* Buffer, int32_t BufferSize) {
	if (!Port->connected) {
		std::cout << "Serial Read: Invalid Serial Port.\n";
		return -1;
	}

	// Sit here doing things for RECV for now.

	//DWORD dwCommEvent;
	//DWORD dwRead;
	//char  chRead[4096];
	//int totalRead = 0;
	//int recvCount = 0;

	//if (!SetCommMask(Port->descriptor, EV_RXCHAR)) {
	//	std::cout << "Failed to set comm mask\n";
	//	return -1;
	//}

	//while (true) {
	//	std::cout << "Waiting for comm event\n";
	//	if (WaitCommEvent(Port->descriptor, &dwCommEvent, NULL)) {
	//		do {
	//			if (ReadFile(Port->descriptor, &chRead, 4096, &dwRead, NULL)) {
	//				// A byte has been read; process it.
	//				totalRead += dwRead;
	//				std::cout << "Read " << dwRead << " " << totalRead << "\n";

	//				for (int i = 0; i < dwRead; ++i) {
	//					uint8_t v = chRead[i];

	//					int newGoal = (recvCount / 256) % 256;

	//					if (v != newGoal) {
	//						std::cout << "Seq failure (" << recvCount << ") expected: " << newGoal << " got: " << (int)v << "\n";
	//						return -1;
	//					}
	//					
	//					recvCount += 1;
	//				}


	//			} else {
	//				// An error occurred in the ReadFile call.
	//				std::cout << "e\n";
	//				break;
	//			}
	//		} while (dwRead);
	//	} else {
	//		// Error in WaitCommEvent
	//		break;
	//	}
	//}

	//return -1;

	OVERLAPPED overlapped = {};
	DWORD bytesRead;

	if (!ReadFile(Port->descriptor, Buffer, BufferSize, &bytesRead, &overlapped)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			std::cout << "Serial Read: IO Pending Error.\n";
			//serialPortDisconnect(Port);
			return -1;
		} else {
			if (!GetOverlappedResult(Port->descriptor, &overlapped, &bytesRead, true)) {
				std::cout << "Serial Read: Waiting Error.\n";
				//serialPortDisconnect(Port);
				return -1;
			} else if (bytesRead > 0) {
				return bytesRead;
			}
		}
	} else if (bytesRead > 0) {
		return bytesRead;
	}

	return 0;
}