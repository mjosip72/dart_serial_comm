
#include <iostream>
#include <Windows.h>
#include <SetupAPI.h>
#include <initguid.h>
#include <devguid.h>
#include <vector>

using namespace std;

void listPorts() {

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

		char friendly_name[256];
		DWORD friendly_name_len = 0;

		bool got_friendly_name = SetupDiGetDeviceRegistryPropertyA(
			device_info_list,
			&device_info_data,
			SPDRP_FRIENDLYNAME,
			NULL,
			(PBYTE)friendly_name,
			256,
			&friendly_name_len
		);

		if (got_friendly_name && friendly_name_len > 0) friendly_name[friendly_name_len - 1] = 0;
		else friendly_name[0] = 0;

		printf("Port %s %s\n", port_name, friendly_name);

	}

	SetupDiDestroyDeviceInfoList(device_info_list);

}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
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
#define PRINT_PORT_CONFIG(x) cout << #x << " = " << (uint64_t)dcb.x << endl
void printPortConfig(HANDLE handle) {

	DCB dcb = { 0 };
	dcb.DCBlength = sizeof(DCB);

	bool success = GetCommState(handle, &dcb);
	if (!success) return;

	PRINT_PORT_CONFIG(BaudRate);
	PRINT_PORT_CONFIG(ByteSize);
	PRINT_PORT_CONFIG(StopBits);
	PRINT_PORT_CONFIG(Parity);

	PRINT_PORT_CONFIG(fParity);
	PRINT_PORT_CONFIG(fBinary);
	PRINT_PORT_CONFIG(fAbortOnError);

	PRINT_PORT_CONFIG(fRtsControl);
	PRINT_PORT_CONFIG(fOutxCtsFlow);
	PRINT_PORT_CONFIG(fOutxDsrFlow);
	PRINT_PORT_CONFIG(fDtrControl);

	PRINT_PORT_CONFIG(fDsrSensitivity);
	PRINT_PORT_CONFIG(fOutX);
	PRINT_PORT_CONFIG(fInX);

	PRINT_PORT_CONFIG(fTXContinueOnXoff);
	PRINT_PORT_CONFIG(fErrorChar);
	PRINT_PORT_CONFIG(fNull);
	PRINT_PORT_CONFIG(XonLim);
	PRINT_PORT_CONFIG(XoffLim);
	PRINT_PORT_CONFIG(XonChar);
	PRINT_PORT_CONFIG(XoffChar);

}

HANDLE handle;

bool openPort(string portName) {

	string portFile = "\\\\.\\" + portName;

	handle = CreateFileA(
		portFile.c_str(),
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

void closePort() {

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

bool configurePortBuffers(int rxBuffer, int txBuffer) {
	return SetupComm(handle, rxBuffer, txBuffer);
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

bool configureEventFlags(DWORD eventFlags) {
	return SetCommMask(handle, eventFlags);
}

int64_t waitForEvent() {

	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DWORD eventMask = 0;
	bool result = WaitCommEvent(handle, &eventMask, &overlapped);

	if (!result) {
		
		DWORD lastError = GetLastError();
		if (lastError == ERROR_IO_PENDING || ERROR_INVALID_PARAMETER) {

			DWORD waitValue = 0;
			do waitValue = WaitForSingleObject(overlapped.hEvent, 500);
			while (waitValue == WAIT_TIMEOUT);

			DWORD numBytesTransferred = 0;
			result = GetOverlappedResult(handle, &overlapped, &numBytesTransferred, FALSE);

			if (waitValue != WAIT_OBJECT_0 || !result) {
				CloseHandle(overlapped.hEvent);
				return 0;
			}

		} else {
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

int64_t readData(uint8_t* buffer, uint64_t count) {

	OVERLAPPED overlapped = {0};
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

int64_t writeData(uint8_t* buffer, uint64_t offset, uint64_t count) {

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

int main() {

	//listPorts();

	openPort("COM3");
	configurePort(115200, 8, 0, 0);
	configurePortBuffers(4096, 4096);
	configureTimeouts();
	configureEventFlags(EV_RXCHAR | EV_ERR);

	closePort();

	system("pause");
	return 0;
}
