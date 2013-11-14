#include "stdafx.h"
#include "Lights.h"

#define TX(x, rect)		(rect.left + (x)*(rect.right-rect.left))
#define TY(y, rect)		(rect.top + (y)*(rect.bottom-rect.top))
#define WIDTH(rect)		(rect.right - rect.left)
#define	HEIGHT(rect)	(rect.bottom - rect.top)

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND hWnd;

FftData fftData, fftLink;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Options(HWND, UINT, WPARAM, LPARAM);

BOOL IsInt(char *str);
BOOL IsFloat(char *str);

BOOL LoadSettings();
BOOL SaveSettings();

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
	HANDLE thread;

	ZeroMemory(fftData.left, sizeof(float)* NSAMPLES / 2);
	ZeroMemory(fftData.right, sizeof(float)* NSAMPLES / 2);

	// Initialize global strings
	strcpy(szTitle, APPTITLE);
	LoadString(hInstance, IDC_LIGHTS, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	fftLink.hWnd = hWnd;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LIGHTS));

	if (!LoadSettings())
		SaveSettings();

	memcpy(fftLink.limit, fftData.limit, sizeof(int)* 4);
	memcpy(fftLink.bass, fftData.bass, sizeof(float)* 4);
	memcpy(fftLink.trebble, fftData.trebble, sizeof(float)* 4);
	strcpy(fftLink.com, fftData.com);

	thread = CreateThread(NULL, 0, &FFTThread, NULL, 0, NULL);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LIGHTS);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON2));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	char buffer[128];

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_FILE_ABOUT:
			strcpy(buffer, APPTITLE);
			strcat(buffer, " was created by Eric Middleton for EE185 at Iowa State University\n\nCopyright (C) 2013 Eric Middleton");
			MessageBox(hWnd, buffer, APPTITLE, MB_OK);
			break;
		case IDM_OPTIONS:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_ERASEBKGND:
		return 1;
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{
			HDC bdc = CreateCompatibleDC(hdc);
			HBITMAP bmp;
			HGDIOBJ bb = GetStockObject(BLACK_BRUSH), gb = GetStockObject(DKGRAY_BRUSH), lg = GetStockObject(GRAY_BRUSH), hb = GetStockObject(HOLLOW_BRUSH), tp = GetStockObject(NULL_PEN),
				rb = CreateSolidBrush(0x000005FF), grb = CreateSolidBrush(0x0005FF05), blb = CreateSolidBrush(0x00FF0505), rp = CreatePen(PS_SOLID, 1, 0x000005FF), blp = CreatePen(PS_SOLID, 1, 0x00FF0505), old, oldBmp, oldPen;
			RECT rect;
			int i;
			float x, y, width = 0.42f / (NSAMPLES / 2 - 1);

			GetClientRect(hWnd, &rect);

			bmp = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
			oldBmp = SelectObject(bdc, bmp);

			old = SelectObject(bdc, gb);
			Rectangle(bdc, rect.left, rect.top, rect.right, rect.bottom);
			SelectObject(bdc, old);

			old = SelectObject(bdc, lg);
			RoundRect(bdc, TX(0.01f, rect), TY(0.1f, rect), TX(0.49f, rect), TY(0.95f, rect), WIDTH(rect) / 75 + 5, HEIGHT(rect) / 75 + 5);
			RoundRect(bdc, TX(0.51f, rect), TY(0.1f, rect), TX(0.99f, rect), TY(0.95f, rect), WIDTH(rect) / 75 + 5, HEIGHT(rect) / 75 + 5);
			SelectObject(bdc, old);

			MoveToEx(bdc, TX(0.05f, rect), TY(0.12f, rect), NULL);
			LineTo(bdc, TX(0.05f, rect), TY(0.85f, rect));
			LineTo(bdc, TX(0.47f, rect), TY(0.85f, rect));

			MoveToEx(bdc, TX(0.55f, rect), TY(0.12f, rect), NULL);
			LineTo(bdc, TX(0.55f, rect), TY(0.85f, rect));
			LineTo(bdc, TX(0.97f, rect), TY(0.85f, rect));

			oldPen = SelectObject(bdc, tp);
			old = SelectObject(bdc, rb);
			Rectangle(bdc, TX(0.02, rect), TY(0.85f - 0.00143137f*fftData.led[0], rect) - 1, TX(0.04f, rect), TY(0.85f, rect));
			Rectangle(bdc, TX(0.52, rect), TY(0.85f - 0.00143137f*fftData.led[1], rect) - 1, TX(0.54f, rect), TY(0.85f, rect));
			SelectObject(bdc, old);

			old = SelectObject(bdc, blb);
			Rectangle(bdc, TX(0.02, rect), TY(0.485f - 0.00143137f*fftData.led[2], rect), TX(0.04f, rect), TY(0.485f, rect) + 1);
			Rectangle(bdc, TX(0.52, rect), TY(0.485f - 0.00143137f*fftData.led[3], rect), TX(0.54f, rect), TY(0.485f, rect) + 1);
			SelectObject(bdc, old);
			SelectObject(bdc, oldPen);

			old = SelectObject(bdc, hb);
			Rectangle(bdc, TX(0.02f, rect), TY(0.12f, rect), TX(0.04f, rect), TY(0.85, rect));
			Rectangle(bdc, TX(0.52f, rect), TY(0.12f, rect), TX(0.54f, rect), TY(0.85, rect));
			SelectObject(bdc, gb);

			MoveToEx(bdc, TX(0.02f, rect), TY(0.485f, rect), NULL);
			LineTo(bdc, TX(0.04f, rect), TY(0.485f, rect));
			MoveToEx(bdc, TX(0.52f, rect), TY(0.485f, rect), NULL);
			LineTo(bdc, TX(0.54f, rect), TY(0.485f, rect));

			old = SelectObject(bdc, bb);
			for (i = 1; i < NSAMPLES/2; i++) {
				x = width * (i-1);
				y = 0.73f*fftData.left[i];
				Rectangle(bdc, TX(0.05f + x, rect), TY(0.85f - y, rect), TX(0.05 + x + width, rect), TY(0.85f, rect));

				y = 0.73f*fftData.right[i];
				Rectangle(bdc, TX(0.55f + x, rect), TY(0.85f - y, rect), TX(0.55 + x + width, rect), TY(0.85f, rect));
			}
			SelectObject(bdc, old);

			oldPen = SelectObject(bdc, rp);
			MoveToEx(bdc, TX(0.05f + width*(fftData.limit[0] - 1), rect), TY(0.85f, rect) + 3, NULL);
			LineTo(bdc, TX(0.05f + width*(fftData.limit[1] - 1), rect) + (fftData.limit[0] == fftData.limit[1]), TY(0.85f, rect) + 3);
			MoveToEx(bdc, TX(0.55f + width*(fftData.limit[0] - 1), rect), TY(0.85f, rect) + 3, NULL);
			LineTo(bdc, TX(0.55f + width*(fftData.limit[1] - 1), rect) + (fftData.limit[0] == fftData.limit[1]), TY(0.85f, rect) + 3);
			SelectObject(bdc, oldPen);

			oldPen = SelectObject(bdc, blp);
			MoveToEx(bdc, TX(0.05f + width*(fftData.limit[2] - 1), rect), TY(0.85f, rect) + 2, NULL);
			LineTo(bdc, TX(0.05f + width*(fftData.limit[3] - 1), rect) + (fftData.limit[2] == fftData.limit[3]), TY(0.85f, rect) + 2);
			MoveToEx(bdc, TX(0.55f + width*(fftData.limit[2] - 1), rect), TY(0.85f, rect) + 2, NULL);
			LineTo(bdc, TX(0.55f + width*(fftData.limit[3] - 1), rect) + (fftData.limit[2] == fftData.limit[3]), TY(0.85f, rect) + 2);
			SelectObject(bdc, oldPen);

			if (fftData.connected)
				old = SelectObject(bdc, grb);
			else
				old = SelectObject(bdc, rb);
			Ellipse(bdc, TX(0.99f, rect) - TY(0.048f, rect), TY(0.951f, rect), TX(0.99f, rect), TY(0.999f, rect));
			SelectObject(bdc, old);

			BitBlt(hdc, rect.left, rect.top, rect.right, rect.bottom, bdc, 0, 0, SRCCOPY);
			SelectObject(bdc, oldBmp);
			DeleteObject(bmp);
			DeleteObject(bdc);

			DeleteObject(bb);
			DeleteObject(gb);
			DeleteObject(lg);
			DeleteObject(rb);
			DeleteObject(blb);
			DeleteObject(rp);
			DeleteObject(blp);
			DeleteObject(grb);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_FFT:
		memcpy(fftData.left, fftLink.left, sizeof(float)*NSAMPLES / 2);
		memcpy(fftData.right, fftLink.right, sizeof(float)*NSAMPLES / 2);
		memcpy(fftData.led, fftLink.led, 4);
		fftData.connected = fftLink.connected;

		InvalidateRect(hWnd, NULL, 0);

		if (fftData.set == TRUE) {
			fftData.set = FALSE;
			fftLink.set = TRUE;

			memcpy(fftLink.limit, fftData.limit, sizeof(int)* 4);
			memcpy(fftLink.bass, fftData.bass, sizeof(float)* 4);
			memcpy(fftLink.trebble, fftData.trebble, sizeof(float)* 4);
			strcpy(fftLink.com, fftData.com);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	char buffer[128], com[128];

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_BLOW, _itoa(fftLink.limit[0], buffer, 10));
		SetDlgItemText(hDlg, IDC_BHIGH, _itoa(fftLink.limit[1], buffer, 10));
		SetDlgItemText(hDlg, IDC_TLOW, _itoa(fftLink.limit[2], buffer, 10));
		SetDlgItemText(hDlg, IDC_THIGH, _itoa(fftLink.limit[3], buffer, 10));
		SetDlgItemText(hDlg, IDC_COM, fftLink.com);

		sprintf(buffer, "%.2f", fftLink.bass[0]);
		SetDlgItemText(hDlg, IDC_BMULT, buffer);

		sprintf(buffer, "%.2f", fftLink.bass[1]);
		SetDlgItemText(hDlg, IDC_BAVWT, buffer);

		sprintf(buffer, "%.2f", fftLink.bass[2]);
		SetDlgItemText(hDlg, IDC_BMXWT, buffer);

		sprintf(buffer, "%.2f", fftLink.bass[3]);
		SetDlgItemText(hDlg, IDC_BTHRSH, buffer);

		sprintf(buffer, "%.2f", fftLink.trebble[0]);
		SetDlgItemText(hDlg, IDC_TMULT, buffer);

		sprintf(buffer, "%.2f", fftLink.trebble[1]);
		SetDlgItemText(hDlg, IDC_TAVWT, buffer);

		sprintf(buffer, "%.2f", fftLink.trebble[2]);
		SetDlgItemText(hDlg, IDC_TMXWT, buffer);

		sprintf(buffer, "%.2f", fftLink.trebble[3]);
		SetDlgItemText(hDlg, IDC_TTHRSH, buffer);

		return (INT_PTR)TRUE;
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDOK) {
			int tLimits[4];
			float tBass[4], tTrebble[4];

			GetDlgItemText(hDlg, IDC_BLOW, buffer, 128);
			if (!IsInt(buffer, &(tLimits[0])) || (tLimits[0] < 1) || (tLimits[0] > 256)) {
				MessageBox(hDlg, "Bass Lowest Bin must be a valid number between 1 and 256!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_TLOW, buffer, 128);
			if (!IsInt(buffer, &(tLimits[2])) || (tLimits[2] < 1) || (tLimits[2] > 256)) {
				MessageBox(hDlg, "Trebble Lowest Bin must be a valid number between 1 and 256!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_BHIGH, buffer, 128);
			if (!IsInt(buffer, &(tLimits[1])) || (tLimits[1] < 1) || (tLimits[1] > 256)) {
				MessageBox(hDlg, "Bass Highest Bin must be a valid number between 1 and 256!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_THIGH, buffer, 128);
			if (!IsInt(buffer, &(tLimits[3])) || (tLimits[3] < 1) || (tLimits[3] > 256)) {
				MessageBox(hDlg, "Trebble Highest Bin must be a valid number between 1 and 256!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}

			GetDlgItemText(hDlg, IDC_BMULT, buffer, 128);
			if (!IsFloat(buffer, &(tBass[0])) || (tBass[0] < 0)) {
				MessageBox(hDlg, "Bass Multiplier must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}

			GetDlgItemText(hDlg, IDC_TMULT, buffer, 128);
			if (!IsFloat(buffer, &(tTrebble[0])) || (tTrebble[0] < 0)) {
				MessageBox(hDlg, "Trebble Multiplier must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_BAVWT, buffer, 128);
			if (!IsFloat(buffer, &(tBass[1])) || (tBass[1] < 0)) {
				MessageBox(hDlg, "Bass Average Weight must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_TAVWT, buffer, 128);
			if (!IsFloat(buffer, &(tTrebble[1])) || (tTrebble[1] < 0)) {
				MessageBox(hDlg, "Trebble Average Weight must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_BMXWT, buffer, 128);
			if (!IsFloat(buffer, &(tBass[2])) || (tBass[2] < 0)) {
				MessageBox(hDlg, "Bass Maximum Weight must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_TMXWT, buffer, 128);
			if (!IsFloat(buffer, &(tTrebble[2])) || (tTrebble[2] < 0)) {
				MessageBox(hDlg, "Trebble Maximum Weight must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_BTHRSH, buffer, 128);
			if (!IsFloat(buffer, &(tBass[3])) || (tBass[3] < 0)) {
				MessageBox(hDlg, "Bass Lighting Threshold must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}
			GetDlgItemText(hDlg, IDC_TTHRSH, buffer, 128);
			if (!IsFloat(buffer, &(tTrebble[3])) || (tTrebble[3] < 0)) {
				MessageBox(hDlg, "Trebble Lighting Threshold must be a valid positive number!", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}

			GetDlgItemText(hDlg, IDC_COM, buffer, 128);
			if (strlen(strcpy(com, buffer)) <= 0) {
				MessageBox(hDlg, "Please specify a COM port.", "ERROR!", MB_ICONERROR);
				return (INT_PTR)TRUE;
			}

			fftData.set = TRUE;
			memcpy(fftData.limit, tLimits, sizeof(int)* 4);
			memcpy(fftData.bass, tBass, sizeof(float)* 4);
			memcpy(fftData.trebble, tTrebble, sizeof(float)* 4);
			strcpy(fftData.com, com);

			SaveSettings();

			EndDialog(hDlg, LOWORD(wParam));
			
			return (INT_PTR)TRUE;
		}
		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_T1) || (HWND)lParam == GetDlgItem(hDlg, IDC_T2) || (HWND)lParam == GetDlgItem(hDlg, IDC_T3) || (HWND)lParam == GetDlgItem(hDlg, IDC_T4) ||
			(HWND)lParam == GetDlgItem(hDlg, IDC_T5) || (HWND)lParam == GetDlgItem(hDlg, IDC_T6) || (HWND)lParam == GetDlgItem(hDlg, IDC_T7) || (HWND)lParam == GetDlgItem(hDlg, IDC_T8) ||
			(HWND)lParam == GetDlgItem(hDlg, IDC_T9)) {
			SetTextColor((HDC)wParam, 0x00AAAAAA);
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (BOOL)GetStockObject(NULL_BRUSH);
		}
		return FALSE;
		break;
	case WM_PAINT: {
			hdc = BeginPaint(hDlg, &ps);
			HDC bdc = CreateCompatibleDC(hdc);
			HBITMAP bmp;
			HGDIOBJ gb = GetStockObject(DKGRAY_BRUSH), lg = GetStockObject(GRAY_BRUSH),
				old, oldBmp;
			RECT rect;

			GetClientRect(hDlg, &rect);

			bmp = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
			oldBmp = SelectObject(bdc, bmp);

			old = SelectObject(bdc, gb);
			Rectangle(bdc, rect.left, rect.top, rect.right, rect.bottom);
			SelectObject(bdc, old);

			BitBlt(hdc, rect.left, rect.top, rect.right, rect.bottom, bdc, 0, 0, SRCCOPY);
			SelectObject(bdc, oldBmp);
			DeleteObject(gb);
			DeleteObject(bdc);
			DeleteObject(bmp);
			DeleteObject(lg);
		}
		break;
	}
	return (INT_PTR)FALSE;
}

BOOL IsInt(char *str, int *out) {
	char *end;
	int i;

	errno = 0;
	i = strtoul(str, &end, 10);

	if (out != NULL)
		*out = i;

	return (errno == 0) && (*end == '\0') && (strlen(str) > 0);
}

BOOL IsFloat(char *str, float *out) {
	char *end;
	float f;

	errno = 0;
	f = strtod(str, &end);

	if (out != NULL)
		*out = f;

	return (errno == 0) && (*end == '\0') && (strlen(str) > 0);
}

BOOL LoadSettings() {
	HANDLE hFile = CreateFile("lights.dat", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD nRead = 0;
	BYTE data[FSIZE + MAXCOM];
	BOOL fail = FALSE;

	if (hFile != INVALID_HANDLE_VALUE) {
		ReadFile(hFile, data, FSIZE + MAXCOM, &nRead, NULL);

		if (nRead < (FSIZE + 1))
			fail = TRUE;
		else {
			memcpy(fftData.limit, data, sizeof(int)* 4);
			memcpy(fftData.bass, data + sizeof(int)* 4, sizeof(float)*4);
			memcpy(fftData.trebble, data + sizeof(int)* 4 + sizeof(float)* 4, sizeof(float)*4);
			
			strcpy(fftData.com, data + FSIZE);
		}
		CloseHandle(hFile);
	}
	else {
		fail = TRUE;
	}
	
	if (fail == TRUE) {
		MessageBox(hWnd, "Settings file not found.\nDefault settings will be used.", APPTITLE, MB_ICONERROR);

		fftData.limit[0] = 1;
		fftData.limit[1] = 1;
		fftData.limit[2] = 40;
		fftData.limit[3] = 150;

		fftData.bass[0] = 450.f;
		fftData.bass[1] = 1.f;
		fftData.bass[2] = 0.f;
		fftData.bass[3] = 80.f;

		fftData.trebble[0] = 1250.f;
		fftData.trebble[1] = 0.5f;
		fftData.trebble[2] = 1.5f;
		fftData.trebble[3] = 80.f;

		strcpy(fftData.com, "COM3");
	}

	return fail == FALSE;
}

BOOL SaveSettings() {
	HANDLE hFile = CreateFile("lights.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD nWritten = 0;
	BYTE data[FSIZE + MAXCOM];
	BOOL fail = FALSE;

	if (hFile != INVALID_HANDLE_VALUE) {
		WriteFile(hFile, fftData.limit, sizeof(int)* 4, &nWritten, NULL);
		if (nWritten != (sizeof(int)* 4))
			fail = TRUE;
		else {
			WriteFile(hFile, fftData.bass, sizeof(float)* 4, &nWritten, NULL);
			if (nWritten != (sizeof(float)* 4))
				fail = TRUE;
			else {
				WriteFile(hFile, fftData.trebble, sizeof(float)* 4, &nWritten, NULL);
				if (nWritten != (sizeof(float)* 4))
					fail = TRUE;
				else {
					WriteFile(hFile, fftData.com, strlen(fftData.com) + 1, &nWritten, NULL);
					if (nWritten != (strlen(fftData.com) + 1))
						fail = TRUE;
				}
			}
		}
		CloseHandle(hFile);
	}
	else {
		fail = TRUE;
	}

	if (fail == TRUE) {
		MessageBox(hWnd, "Unable to write settings file!", APPTITLE, MB_ICONERROR);
	}

	return fail == FALSE;
}