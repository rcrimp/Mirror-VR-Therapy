#include <Windows.h>
#include <iostream>

#include <Setupapi.h>
#include <Hidsdi.h>

//#include <devguid.h>
//#include <regstr.h>

/* Link:
* SetupAPI.lib
* Hid.lib
*/

using namespace std;

size_t stringLength(WCHAR* str){
	size_t c = 0;
	while (true){
		if (str[c] == '\0')
			return c;
		if (c++ > MAX_PATH)
			return MAX_PATH;
	}
}

void stringCopy(WCHAR* src, WCHAR *dst){
	unsigned int c = 0;
	while (true){
		dst[c] = src[c];
		if (dst[c] = '\0')
			return;
		if (c++ > MAX_PATH)
			return;
	}
}

BOOL findSubstring(WCHAR *full_string, const char *substring){
	unsigned int c = 0, d = 0;
	while (true) {
		if (substring[d] == '\0')
			return true;
		if (full_string[c] == '\0')
			return false;
		d = (full_string[c] == substring[d]) ? d + 1 : 0;
		// since full_string will should be a path name, we can use MAX_PATH as an arbitrary limit
		if (c++ > MAX_PATH)
			return false;
	}
}

WCHAR* getHIDPath(unsigned int vid, unsigned int pid){
	WCHAR* result = NULL;
	GUID guid;
	HDEVINFO hDeviceInfo;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	DWORD i;

	HidD_GetHidGuid(&guid);
	hDeviceInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDeviceInfo == INVALID_HANDLE_VALUE)
	{
		cerr << "Failed to get Device Handle" << endl;
		return result;
	}

	i = 0;
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	while (SetupDiEnumDeviceInterfaces(hDeviceInfo, NULL, &guid, i++, &deviceInterfaceData)){
		PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetail;
		ULONG requiredSize;

		//Get the details with null values to get the required size of the buffer
		SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, NULL, 0, &requiredSize, 0);

		deviceDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredSize);
		deviceDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

		//Fill the buffer with the device details
		if (!SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, deviceDetail, requiredSize, &requiredSize, NULL)){
			SetupDiDestroyDeviceInfoList(hDeviceInfo);
			free(deviceDetail);
		}

		if (findSubstring(deviceDetail->DevicePath, "vid_2833") && findSubstring(deviceDetail->DevicePath, "pid_0021")){
			//cout << stringLength(deviceDetail->DevicePath) << endl;
			//cout << sizeof(WCHAR) << endl;
			//wcout << deviceDetail->DevicePath << endl;
			//result = new WCHAR[(stringLength(deviceDetail->DevicePath) * 3 + 11)];// (WCHAR*)malloc(sizeof(WCHAR) * (stringLength(deviceDetail->DevicePath) + 1));
			return deviceDetail->DevicePath;
			//stringCopy(deviceDetail->DevicePath, result);
		}
		free(deviceDetail);
	}
	SetupDiDestroyDeviceInfoList(hDeviceInfo);
	return result;
}
//\\ ? \hid#vid_2833&pid_0021#7 & 3e4f5e5 & 0 & 0000#{4d1e55b2 - f16f - 11cf - 88cb - 001111000030}
typedef struct LED_struct {
	double position[3];
	double normal[3];
} LED_t;

void printLED(LED_t led){
	cout << " (" << led.position[0] << ", "
		<< led.position[1] << ", "
		<< led.position[2] << ")" << endl;
}

int main()
{
	system("taskkill /F /IM OVRServer_x64.exe");
	system("taskkill /F /IM OVRServer_x86.exe");
	WCHAR* devicePath = getHIDPath(0x2833U, 0x0021U);
	if (devicePath == NULL){
		cerr << "unable to find Oculus Rift USB Device" << endl;
		return 1;
	}

	HANDLE HIDfile = CreateFile(devicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (GetLastError() != 0){
		cerr << "unable to obtain read permissions of the Oculus Rift USB device, please close the OVR Service and run again" << endl;
		//free(devicePath);
		return 1;
	}

	unsigned int numReports = 0;
	unsigned int firstReportIndex = 0;
	unsigned int numLED = 0;
	LED_t *LEDarray;
	LED_t imu;
	BYTE buffer[30];
	memset(buffer, 0, sizeof(buffer[0]));
	buffer[0] = 0x0FU;

	while (true){
		if (!HidD_GetFeature(HIDfile, buffer, sizeof(buffer)))
			break;
		unsigned int ri = buffer[24];
		unsigned int nr = buffer[26];

		if (numReports == 0) // Is this the first received report?
		{
			numReports = nr;
			firstReportIndex = ri;

			/* Allocate the marker array: */
			numLED = numReports - 1; // One of the reports defines the IMU's position
			LEDarray = new LED_t[numLED];
		}
		else if (ri == firstReportIndex) // Is this the first received report again?
			break;

		/* Helper structure to convert two little-endian bytes to a 16-bit signed integer: */
		union{
			UINT8 b[2];
			INT16 i;
		} convert16;

		/* Helper structure to convert four little-endian bytes to a 32-bit signed integer: */
		union{
			UINT8 b[4];
			INT32 i;
		} convert32;

		/* Extract the LED position: */
		BYTE* bufPtr = buffer + 4;
		for (int i = 0; i < 3; ++i)	{
			/* Convert four little-endian bytes to a 32-bit signed integer: */
			for (int j = 0; j < 4; ++j, ++bufPtr)
				convert32.b[j] = *bufPtr;
			if (buffer[3] == 0x02U) // if it's an LED
				LEDarray[ri].position[i] = (convert32.i); // Convert from micrometers to meters
			else // else it's the IMU
				imu.position[i] = (convert32.i);
		}

		/* Extract the LED's direction vector: */
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 2; ++j, ++bufPtr)
				convert16.b[j] = *bufPtr;
			if (buffer[3] == 0x02U) { //if it's an LED
				LEDarray[ri].normal[i] = convert16.i;
			}
			//normalize([markerIndex].normal[0], [markerIndex].normal[1], [markerIndex].normal[2]);
		}
	}

	//cout << "IMU:"; printLED(imu);
	for (unsigned int i = 0; i < numLED; i++){
		cout << "marker " << i; printLED(LEDarray[i]);
	}

	//system("%programfiles(x86)%\Oculus\Service\OVRServer.exe -start");
	cout << "Finished, Press Enter to quit" << endl;
	//free(devicePath);
	getchar();
	return 0;
}