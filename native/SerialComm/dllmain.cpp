// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#define DllExport extern "C" __declspec(dllexport)

struct serial_port_info_t {
	char* name;
	char* description;
};

std::vector<serial_port_info_t> ports;
HANDLE handle;


DllExport int listPorts() {

	HDEVINFO device_info_list = SetupDiGetClassDevs(
		(const GUID*)&GUID_DEVCLASS_PORTS,
		NULL,
		NULL,
		DIGCF_PRESENT
	);

	int device_info_list_index = 0;
	SP_DEVINFO_DATA device_info_data;

	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

	while (SetupDiEnumDeviceInfo(device_info_list, device_info_list_index, &device_info_data)) {
		device_info_list_index++;

		HKEY hKey = SetupDiOpenDevRegKey(
			device_info_list,
			&device_info_data,
			DICS_FLAG_GLOBAL,
			0,
			DIREG_DEV,
			KEY_READ
		);

		char port_name[256];
		DWORD port_name_len = 256;

		LONG return_code = RegQueryValueExA(
			hKey,
			"PortName",
			NULL,
			NULL,
			(LPBYTE)port_name,
			&port_name_len
		);

		RegCloseKey(hKey);

		if (return_code != EXIT_SUCCESS) continue;

		if (port_name_len > 0 && port_name_len <= 256) port_name[port_name_len - 1] = 0;
		else continue;

		if (strncmp(port_name, "LPT", 3) == 0) continue;

		char description[256];
		DWORD description_len = 0;

		bool got_description = SetupDiGetDeviceRegistryPropertyA(
			device_info_list,
			&device_info_data,
			SPDRP_FRIENDLYNAME,
			NULL,
			(PBYTE)description,
			256,
			&description_len
		);

		if (got_description && description_len > 0) description[description_len - 1] = 0;
		else description[0] = 0;


		serial_port_info_t port_info;
		
		port_info.name = new char[port_name_len];
		memcpy(port_info.name, port_name, port_name_len);

		port_info.description = new char[description_len];
		memcpy(port_info.description, description, description_len);

		ports.emplace_back(port_info);

	}

	SetupDiDestroyDeviceInfoList(device_info_list);
	return ports.size();

}

DllExport char* retrievePortName(int i) {
	return ports[i].name;
}

DllExport char* retrievePortDescription(int i) {
	return ports[i].description;
}

DllExport void clearPortInfo() {
	for (int i = 0; i < ports.size(); i++) {
		delete[] ports[i].name;
		delete[] ports[i].description;
	}
	ports.clear();
}


DllExport bool openPort(char* portName) {

	handle = CreateFileA(
		portName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
		NULL
	);

	if (handle == INVALID_HANDLE_VALUE) return false;
	return true;

}

DllExport void closePort() {

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(handle, &timeouts);

	PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
	CancelIoEx(handle, NULL);
	SetCommMask(handle, 0);
	CloseHandle(handle);

}


bool configurePort(int baudRate, uint8_t byteSize, uint8_t stopBits, uint8_t parity) {

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(handle, &dcb)) return false;

	dcb.BaudRate = baudRate;
	dcb.ByteSize = byteSize;
	dcb.StopBits = stopBits;
	dcb.Parity = parity;

	dcb.fParity = parity == 0 ? FALSE : TRUE;
	dcb.fBinary = TRUE;
	dcb.fAbortOnError = FALSE;

	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;

	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;

	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;

	dcb.fTXContinueOnXoff = TRUE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.XonLim = 2048;
	dcb.XoffLim = 512;
	dcb.XonChar = 17;
	dcb.XoffChar = 19;

	return SetCommState(handle, &dcb);

}

bool configureTimeouts() {

	COMMTIMEOUTS timeouts = { 0 };
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 1000;
	timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	return SetCommTimeouts(handle, &timeouts);

}

bool configureEventFlags() {
	return SetCommMask(handle, EV_RXCHAR | EV_ERR);
}

DllExport bool simpleConfigure(int baudRate, uint8_t byteSize, uint8_t stopBits, uint8_t parity) {
	return configurePort(baudRate, byteSize, stopBits, parity) && configureTimeouts() && configureEventFlags();
}

DllExport bool configurePortBuffers(int rxBuffer, int txBuffer) {
	return SetupComm(handle, rxBuffer, txBuffer);
}


DllExport int64_t waitForEvent() {

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DWORD eventMask = 0;
	bool result = WaitCommEvent(handle, &eventMask, &overlapped);

	if (!result) {

		DWORD lastError = GetLastError();
		if (lastError == ERROR_IO_PENDING || lastError == ERROR_INVALID_PARAMETER) {

			DWORD waitValue = 0;
			do waitValue = WaitForSingleObject(overlapped.hEvent, 500);
			while (waitValue == WAIT_TIMEOUT);

			DWORD numBytesTransferred = 0;
			result = GetOverlappedResult(handle, &overlapped, &numBytesTransferred, FALSE);

			if (waitValue != WAIT_OBJECT_0 || !result) {
				CloseHandle(overlapped.hEvent);
				return 0;
			}

		}
		else {
			CloseHandle(overlapped.hEvent);
			return -1;
		}

	}

	COMSTAT stat;
	DWORD errorMask = 0;
	result = ClearCommError(handle, &errorMask, &stat);
	if (!result) return -1;

	CloseHandle(overlapped.hEvent);

	if (errorMask != 0) return 0;
	if (eventMask & EV_RXCHAR) return stat.cbInQue;
	return 0;

}

DllExport int64_t readData(uint8_t* buffer, uint64_t count) {

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	bool result = ReadFile(handle, buffer, count, NULL, &overlapped);
	if (!result && GetLastError() != ERROR_IO_PENDING) {
		CloseHandle(overlapped.hEvent);
		return -1;
	}

	DWORD numBytesRead = 0;
	result = GetOverlappedResult(handle, &overlapped, &numBytesRead, TRUE);

	CloseHandle(overlapped.hEvent);

	if (!result) return -1;
	return numBytesRead;

}

DllExport int64_t writeData(uint8_t* buffer, uint64_t offset, uint64_t count) {

	OVERLAPPED overlapped;
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	bool result = WriteFile(handle, buffer + offset, count, NULL, &overlapped);
	if (!result && GetLastError() != ERROR_IO_PENDING) {
		CloseHandle(overlapped.hEvent);
		return -1;
	}

	DWORD numBytesWritten = 0;
	result = GetOverlappedResult(handle, &overlapped, &numBytesWritten, TRUE);

	CloseHandle(overlapped.hEvent);

	if (!result) return -1;
	return numBytesWritten;

}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) { return TRUE; }
