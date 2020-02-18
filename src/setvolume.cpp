#define WINVER       0x0A00 // windows10
#define _WIN32_WINNT 0x0A00 // windows10

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <wchar.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

enum SetVolumeError {
	SETVOL_E_OK,
	SETVOL_E_INVALID_ARGS,
	SETVOL_E_COINITIALIZEEXFAILED,
	SETVOL_E_GET_ENUMURATOR,
	SETVOL_E_GET_DEVIVE,
	SETVOL_E_CREATE_VOLUMEOBJECT,
	SETVOL_E_GET_MUTE,
	SETVOL_E_GET_VOLUME,
	SETVOL_E_SET_MUTE,
	SETVOL_E_SET_VOLUME,
};

void showusage()
{
	fwprintf_s(stderr, L"usage: setvolume <mute:0-1> <volume:0-100>\n");
}

int getdigit(WCHAR c)
{
	if (c < L'0' || c > L'9') return -1;
	return ((int)c - (int)L'0');
}

bool getargs(int argc, WCHAR* argv[], bool &mute, int &volume)
{
	int d;
	int l;

	if (3 != argc) {
		return SETVOL_E_INVALID_ARGS;
	}

	// check length
	if(wcslen(argv[1]) != 1) return false;

	// check argument value
	d = getdigit(argv[1][0]);
	if (d < 0) return false;
	if (d > 1) return false;
	mute = d == 1 ? true : false;

	// check argument value
	l = (int)wcslen(argv[2]);
	if (l >= 4) return false;
	volume = 0;
	for (int i = 0; i < l; ++i)
	{
		d = getdigit(argv[2][i]);

		// maybe start with 0(ex. 01 020)
		if (d == 0 && volume == 0 && l != 1) return false;

		volume *= 10;
		volume += d;
	}
	if (volume > 100) return false;

	return true;
}

int wmain(int argc, WCHAR* argv[])
{
	HRESULT hr;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioEndpointVolume* pVolume = NULL;
	bool mute = false;
	int volume = 0;
	int rc = SETVOL_E_OK;
	BOOL currentMute;
	float currentVolume;

	if (!getargs(argc, argv, mute, volume))
	{
		showusage();
		return SETVOL_E_INVALID_ARGS;
	}

#ifdef _DEBUG
	fwprintf_s(stdout, L"mute: %s, volume: %d\n", mute ? L"true" : L"false", volume);
	return 0;
#endif

	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		return SETVOL_E_COINITIALIZEEXFAILED;
	}

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&pEnumerator));
	if (FAILED(hr)) {
		rc = SETVOL_E_GET_ENUMURATOR;
		goto end;
	}

	// get default multimedia devices
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	if (FAILED(hr)) {
		rc = SETVOL_E_GET_DEVIVE;
		goto end;
	}

	// create volume object
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVolume);
	if (FAILED(hr)) {
		rc = SETVOL_E_CREATE_VOLUMEOBJECT;
		goto end;
	}

	// get current mute status
	hr = pVolume->GetMute(&currentMute);
	if (FAILED(hr)) {
		rc = SETVOL_E_GET_MUTE;
		goto end;
	}

	// get current volume
	hr = pVolume->GetMasterVolumeLevelScalar(&currentVolume);
	if (FAILED(hr)) {
		rc = SETVOL_E_GET_VOLUME;
		goto end;
	}

	// set mute status
	hr = pVolume->SetMute(mute ? TRUE : FALSE, &GUID_NULL);
	if (FAILED(hr)) {
		rc = SETVOL_E_SET_MUTE;
		goto end;
	}
	
	// set mute status
	hr = pVolume->SetMasterVolumeLevelScalar((float)volume / 100.0f, &GUID_NULL);
	if (FAILED(hr)) {
		rc = SETVOL_E_SET_VOLUME;
		goto end;
	}

	// show result
	fwprintf_s(stdout, L"mute: %s -> %s\n", currentMute ? L"true" : L"false", mute ? L"true" : L"false");
	fwprintf_s(stdout, L"volume: %0.0f -> %d\n", currentVolume * 100., volume);

end:
	if (pVolume)
		pVolume->Release();
	if (pDevice)
		pDevice->Release();
	if (pEnumerator)
		pEnumerator->Release();

	CoUninitialize();

	return rc;
}
