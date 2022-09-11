#pragma once

struct ldiSerialPort {
	HANDLE descriptor = INVALID_HANDLE_VALUE;
	bool connected = false;
};

bool serialPortDisconnect(ldiSerialPort* Port) {
	if (Port->descriptor != INVALID_HANDLE_VALUE)
		CloseHandle(Port->descriptor);

	Port->descriptor = INVALID_HANDLE_VALUE;
	Port->connected = false;

	return true;
}

bool serialPortConnect(ldiSerialPort* Port, const char* Name, int BitRate) {
	serialPortDisconnect(Port);
	std::cout << "Attempt com connection: " << Name << "\n";

	HANDLE handle = CreateFile(Name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if (handle == INVALID_HANDLE_VALUE) {
		std::cout << "Serial Connect: Failed to open.\n";
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
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 10;
	//timeouts.WriteTotalTimeoutConstant = 0;
	//timeouts.WriteTotalTimeoutMultiplier = 0;
	SetCommTimeouts(handle, &timeouts);

	Port->descriptor = handle;
	Port->connected = true;
	std::cout << "Serial Connect: Connected to " << Name << ".\n";

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
		if (GetLastError() != ERROR_IO_PENDING) {
			std::cout << "Serial Write: Write Failed.\n";
			serialPortDisconnect(Port);
			return -1;
		} else {
			if (!GetOverlappedResult(Port->descriptor, &overlapped, &bytesWritten, true)) {
				std::cout << "Serial Write: Waiting Error.\n";
				serialPortDisconnect(Port);
				return -1;
			} else {
				if (bytesWritten != BufferSize) {
					std::cout << "Serial Write: Failed to write all bytes.\n";
					serialPortDisconnect(Port);
					return -1;
				}

				return bytesWritten;
			}
		}
	} else {
		if (bytesWritten != BufferSize) {
			std::cout << "Serial Write: Failed to write all bytes.\n";
			serialPortDisconnect(Port);
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

	OVERLAPPED overlapped = {};
	DWORD bytesRead;

	if (!ReadFile(Port->descriptor, Buffer, BufferSize, &bytesRead, &overlapped)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			std::cout << "Serial Read: IO Pending Error.\n";
			serialPortDisconnect(Port);
			return -1;
		} else {
			if (!GetOverlappedResult(Port->descriptor, &overlapped, &bytesRead, true)) {
				std::cout << "Serial Read: Waiting Error.\n";
				serialPortDisconnect(Port);
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