#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "Lights.h"

#define MOD				4

#define MAG(cpx)		(sqrt(cpx.r*cpx.r + cpx.i*cpx.i)/NSAMPLES)

void GetLeftSamples(short *in, short *out, int length);
void GetRightSamples(short *in, short *out, int length);
void FillCpx(short *in, kiss_fft_cpx *out, int length);
void ScaleCpx(kiss_fft_cpx *cpx);
void SendFft(HWND hWnd, LPHANDLE hPtr, kiss_fft_cpx *left, kiss_fft_cpx *right, byte *leds);
void OpenCom(LPHANDLE hPtr);

int limit[4];
float bass[4], trebble[4];
char com[32];

BOOL IsComConnected(char *com);

DWORD WINAPI FFTThread(LPVOID param) {
	HANDLE hCom;
	HWND hWnd;
	HWAVEIN lineIn;
	WAVEFORMATEX waveFormat;
	WAVEHDR waveHeader;
	short *data, left[NSAMPLES], right[NSAMPLES];
	byte serial[5];
	DWORD nSent;
	UINT ret;
	kiss_fft_cfg fft = kiss_fft_alloc(NSAMPLES, 0, 0, 0);
	kiss_fft_cpx inCpx[NSAMPLES], leftOut[NSAMPLES], rightOut[NSAMPLES];
	BYTE count = 0, error = FALSE, failCount = 0;

	hWnd = fftLink.hWnd;

	memcpy(limit, fftLink.limit, sizeof(int)* 4);
	memcpy(bass, fftLink.bass, sizeof(float)* 4);
	memcpy(trebble, fftLink.trebble, sizeof(float)* 4);
	strcpy(com, fftLink.com);

	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 2;
	waveFormat.nSamplesPerSec = 44100L;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nBlockAlign = waveFormat.nChannels*waveFormat.wBitsPerSample >> 3;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec*waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;

	data = (short*)malloc(waveFormat.nBlockAlign * NSAMPLES);

	waveHeader.lpData = (LPSTR)data;
	waveHeader.dwBufferLength = NSAMPLES*waveFormat.nBlockAlign;
	waveHeader.dwBytesRecorded = 0;
	waveHeader.dwUser = 0;
	waveHeader.dwFlags = 0;
	waveHeader.dwLoops = 0;
	waveHeader.lpNext = 0;

	ret = waveInOpen(&lineIn, WAVE_MAPPER, &waveFormat, 0, 0, WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE);

	if (ret != MMSYSERR_NOERROR) {
		MessageBox(hWnd, "Error opening default recording device!", APPTITLE, MB_ICONERROR);
		return 0;
	}

	ret = waveInPrepareHeader(lineIn, &waveHeader, sizeof(waveHeader));

	if (ret != MMSYSERR_NOERROR) {
		MessageBox(hWnd, "Error preparing default recording device!", APPTITLE, MB_ICONERROR);
		return 0;
	}

	ret = waveInAddBuffer(lineIn, &waveHeader, sizeof(waveHeader));

	if (ret != MMSYSERR_NOERROR) {
		MessageBox(hWnd, "Error preparing default recording device!", APPTITLE, MB_ICONERROR);
		return 0;
	}

	ret = waveInStart(lineIn);

	if (ret != MMSYSERR_NOERROR) {
		MessageBox(hWnd, "Error starting default recording device!", APPTITLE, MB_ICONERROR);
		return 0;
	}

	OpenCom(&hCom);

	fftLink.connected = hCom != INVALID_HANDLE_VALUE;

	while (1) {
		float t = 0, b = 0, m = 0, temp;
		int i;
		while (waveInUnprepareHeader(lineIn, &waveHeader, sizeof(waveHeader)) == WAVERR_STILLPLAYING);

		GetLeftSamples(data, left, NSAMPLES * 2);
		GetRightSamples(data, right, NSAMPLES * 2);

		FillCpx(left, inCpx, NSAMPLES);

		kiss_fft(fft, inCpx, leftOut);

		for (i = limit[0]; i <= limit[1]; i++) {
			temp = MAG(leftOut[i]);
			b += temp;
			m = max(m, temp);
		}
		b = bass[0] * (bass[1] * b / (limit[1] - limit[0] + 1) + bass[2] * m);

		m = 0;
		for (i = limit[2]; i <= limit[3]; i++) {
			temp = MAG(leftOut[i]);
			t += temp;
			m = max(m, temp);
		}
		t = trebble[0] * (trebble[1] * t / (limit[3] - limit[2] + 1) + trebble[2] * m);

		serial[0] = 0;
		serial[1] = min(255, max(0, (b <= bass[3]) ? 1 : (byte)((b - bass[3])*255. / (255 - bass[3]))));
		serial[3] = min(255, max(0, (t <= trebble[3]) ? 1 : (byte)((t - trebble[3])*255. / (255 - trebble[3]))));

		FillCpx(right, inCpx, NSAMPLES);

		kiss_fft(fft, inCpx, rightOut);

		b = 0;
		t = 0;
		m = 0;

		for (i = limit[0]; i <= limit[1]; i++) {
			temp = MAG(rightOut[i]);
			b += temp;
			m = max(m, temp);
		}
		b = bass[0] * (bass[1] * b / (limit[1] - limit[0] + 1) + bass[2] * m);

		m = 0;
		for (i = limit[2]; i <= limit[3]; i++) {
			temp = MAG(rightOut[i]);
			t += temp;
			m = max(m, temp);
		}
		t = trebble[0] * (trebble[1] * t / (limit[3] - limit[2] + 1) + trebble[2] * m);

		serial[2] = min(255, max(0, (b <= bass[3]) ? 1 : (byte)((b - bass[3])*255. / (255 - bass[3]))));
		serial[4] = min(255, max(0, (t <= trebble[3]) ? 1 : (byte)((t - trebble[3])*255. / (255 - trebble[3]))));

		if (hCom != INVALID_HANDLE_VALUE) {
			WriteFile(hCom, serial, 5, &nSent, NULL);

			if (nSent != 5) {
				failCount++;
				if (failCount > 5) {
					CloseHandle(hCom);
					OpenCom(&hCom);

					if (hCom == INVALID_HANDLE_VALUE)
						fftLink.connected = FALSE;
					else
						fftLink.connected = TRUE;
				}
			}
			else
				failCount = 0;
		}

		if (!(count % MOD))
			SendFft(hWnd, &hCom, leftOut, rightOut, serial+1);
		if (hCom == INVALID_HANDLE_VALUE && !(count % 100)) {
			OpenCom(&hCom);

			if (hCom == INVALID_HANDLE_VALUE)
				fftLink.connected == FALSE;
			else
				fftLink.connected = TRUE;
		}
		count++;

		waveInPrepareHeader(lineIn, &waveHeader, sizeof(waveHeader));
		waveInAddBuffer(lineIn, &waveHeader, sizeof(waveHeader));
	}

}

void GetLeftSamples(short *in, short *out, int length) {
	int i;
	for (i = 0; i < (length / 2); i++) {
		out[i] = in[2 * i];
	}
}

void GetRightSamples(short *in, short *out, int length) {
	int i;
	for (i = 0; i < (length/2); i++)
		out[i] = in[2 * i + 1];
}

void FillCpx(short *in, kiss_fft_cpx *out, int length) {
	int i;
	for (i = 0; i < length; i++) {
		out[i].r = (float)in[i] / 32768.f;
		out[i].i = 0;
	}
}

void ScaleCpx(kiss_fft_cpx *cpx, int count) {
	int i;
	for (i = 0; i < count; i++) {

	}
}

void SendFft(HWND hWnd, LPHANDLE hPtr, kiss_fft_cpx *left, kiss_fft_cpx *right, byte *leds) {
	int i;

	for (i = 0; i < NSAMPLES / 2; i++) {
		fftLink.left[i] = min(1.f, 2.*MAG(left[i]));
		fftLink.right[i] = min(1.f, 2.*MAG(right[i]));
	}
	memcpy(fftLink.led, leds, 4);
	
	SendMessage(hWnd, WM_FFT, NULL, NULL);

	if (fftLink.set == TRUE) {
		fftLink.set = FALSE;

		memcpy(limit, fftLink.limit, sizeof(int)* 4);
		memcpy(bass, fftLink.bass, sizeof(float)* 4);
		memcpy(trebble, fftLink.trebble, sizeof(float)* 4);
		
		if (strcmp(com, fftLink.com) != 0) {
			strcpy(com, fftLink.com);
			
			CloseHandle(*hPtr);
			OpenCom(hPtr);

			if (*hPtr == INVALID_HANDLE_VALUE)
				fftLink.connected = FALSE;
			else
				fftLink.connected = TRUE;
		}
	}
}

BOOL IsComConnected(char *com) {
	HKEY hKey;
	int i;
	BOOL found = FALSE;
	char name[128], buffer[128];
	DWORD size = 128, nBuffer, type;

	if (RegOpenKey(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", &hKey) == ERROR_SUCCESS) {
		DWORD nKeys;
		RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &nKeys, NULL, NULL, NULL, NULL);

		for (i = 0; i < nKeys; i++) {
			RegEnumValue(hKey, i, name, &size, NULL, &type, buffer, &nBuffer);
			if (type == REG_SZ && strcmp(com, buffer) == 0) {
				found = TRUE;
				break;
			}
		}
	}

	return found == TRUE;
}

void OpenCom(LPHANDLE hPtr) {
	DCB dcb;

	if (IsComConnected(com))
		*hPtr = CreateFile(com, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	else {
		*hPtr = INVALID_HANDLE_VALUE;
	}

	if (*hPtr != INVALID_HANDLE_VALUE) {
		GetCommState(*hPtr, &dcb);

		dcb.BaudRate = CBR_57600;
		dcb.fBinary = 1;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.ByteSize = 8;

		SetCommState(*hPtr, &dcb);
	}
}