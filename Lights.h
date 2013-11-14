#pragma once
#pragma warning(disable: 4996)
#pragma warning(disable: 4700)

#include "resource.h"
#include "kiss_fft.h"

#include <stdlib.h>
#include <stdio.h>

#define	APPTITLE		"Light Control v1.0"

#define		WM_FFT			(WM_APP+1)
#define		NSAMPLES		512

#define		FSIZE			(4*sizeof(int) + 8*sizeof(float))
#define		MAXCOM			8

#define		FftData_STRING		1
#define		FftData_FFT			2
#define		FftData_ERROR		-1

typedef		char	FftError;

typedef struct DataMessage
{
	LPVOID data;
	BYTE type;
} DataMessage;

typedef struct FftData
{
	HWND hWnd;
	float left[NSAMPLES/2], right[NSAMPLES/2];
	BYTE led[4];
	int limit[4];
	float bass[4], trebble[4];
	char com[MAXCOM], set, connected;
} FftData;

DWORD WINAPI FFTThread(LPVOID param);

extern FftData fftLink;