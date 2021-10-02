#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include <inttypes.h>
#include <process.h>
#include <commctrl.h>


#pragma warning (disable:4996)


BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

enum Color { RED, BLUE };

Color clockColor = BLUE;

HWND hMainWnd = 0;

HANDLE hClock = 0;

bool Terminate = false;

bool ClockPaused = false;

HFONT font;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

	font = CreateFont(25, 0, 0, 0, 900, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, MainWndProc);
	return 0;
}

DWORD WINAPI ClockThread(LPVOID lpParameter) {
	while (!Terminate) {
		char timestr[9];
		SYSTEMTIME time;
		GetLocalTime(&time);
		sprintf(timestr, "%.2d:%.2d:%.2d",
			time.wHour, time.wMinute, time.wSecond);
		SetDlgItemText(hMainWnd, IDC_CLOCK, timestr);
		Sleep(250);
	}
	return 0;
}




void HandleClockPause() {
	if (ClockPaused) {
		

		ResumeThread(hClock);
		clockColor = BLUE;
		ClockPaused = false;
	}
	else {

		SuspendThread(hClock);
		clockColor = RED;
		ClockPaused = true;
	}
}

void DeleteItemFromList(int index) {
	SendDlgItemMessage(
		hMainWnd, IDC_TIMELIST, LB_DELETESTRING, index,
		0);
}



unsigned __int64 SystemTimeToInt(SYSTEMTIME Time) {
	FILETIME ft;
	SystemTimeToFileTime(&Time, &ft);
	ULARGE_INTEGER fti;
	fti.HighPart = ft.dwHighDateTime;
	fti.LowPart = ft.dwLowDateTime;
	return fti.QuadPart;
}



bool RunProcess(LPSTR command) {
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;


	if (CreateProcess(
		NULL,
		command,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi)
		)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);


		return true;
	}
	return false;
}



DWORD WINAPI ScheduleThread(LPVOID lpParameter) {

	SYSTEMTIME selectedTime;
	SendDlgItemMessage(
		hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME, 0,
		(LPARAM)&selectedTime
	);

	SYSTEMTIME currentTime;

	while (true) {
		GetLocalTime(&currentTime);
		if (SystemTimeToInt(currentTime) >= SystemTimeToInt(selectedTime) || Terminate) {
			break;
		}
		Sleep(100);
	}

	char timestr[10];
	sprintf(timestr, "%.2d:%.2d:%.2d",
		selectedTime.wHour, selectedTime.wMinute, selectedTime.wSecond);

	
	LRESULT result =SendDlgItemMessage(
		hMainWnd, IDC_TIMELIST, LB_FINDSTRINGEXACT, 0,
		(LPARAM)timestr
	);

	if(result>=0 && !Terminate) {
		char filename[MAX_PATH] = "";
		GetDlgItemText(hMainWnd, IDC_COMMANDLINE, filename, MAX_PATH);

		RunProcess(filename);

		DeleteItemFromList(result);

	}

	return 0;
}



void HandleDeleteFromList() {
	LRESULT result = SendDlgItemMessage(
		hMainWnd, IDC_TIMELIST, LB_GETCURSEL, 0,
		0
	);

	if (result >= 0) {
		DeleteItemFromList(result);
	}
}



void HandleAddItemToList() {
	SYSTEMTIME selectedTime;
	SendDlgItemMessage(
		hMainWnd, IDC_TIME, DTM_GETSYSTEMTIME, 0,
		(LPARAM)&selectedTime
	);

	SYSTEMTIME currentTime;
	GetLocalTime(&currentTime);
	if (SystemTimeToInt(selectedTime) > SystemTimeToInt(currentTime)) {
		char timestr[9];
		sprintf(timestr, "%.2d:%.2d:%.2d",
			selectedTime.wHour, selectedTime.wMinute, selectedTime.wSecond);



		SendDlgItemMessage(
			hMainWnd, IDC_TIMELIST, LB_ADDSTRING, 0,
			(LPARAM)&timestr
		);


		CloseHandle(CreateThread(NULL, 0, ScheduleThread, NULL, 0, NULL));
	}

}


bool BrowseFileName(HWND hWnd, char* FileName) {
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0"
		"All Files (*.*)\0*.*\0";
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = "exe";
	return GetOpenFileName(&ofn);
}


void HandleBrowse(HWND hWnd) {
	char filename[MAX_PATH] = "";
	if (BrowseFileName(hWnd, filename)) {
		SetDlgItemText(hWnd, IDC_COMMANDLINE, filename);
	}

}


BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

	switch (Msg) {
	case WM_INITDIALOG:
		hMainWnd = hWnd;
		hClock = CreateThread(NULL, 0, ClockThread, NULL, 0, NULL);
		

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			DestroyWindow(hWnd);
			return TRUE;
		case IDC_PAUSE:
			HandleClockPause();
			return TRUE;
		case IDC_BROWSE:
			HandleBrowse(hWnd);
			return TRUE;
		case IDC_ADD:
			HandleAddItemToList();
			return TRUE;
		case IDC_DELETE:
			HandleDeleteFromList();
			return TRUE;
		}
		return FALSE;
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hWnd, IDC_CLOCK))
		{
			
			SetBkMode((HDC)wParam, TRANSPARENT);

			HWND clock = GetDlgItem(hMainWnd, IDC_CLOCK);
			SendMessage(clock, WM_SETFONT, (LPARAM)font, TRUE);

			if (clockColor == BLUE) {
				SetTextColor((HDC)wParam, RGB(0, 250, 171));
			}
			else if (clockColor == RED) {
				SetTextColor((HDC)wParam, RGB(255, 255, 171));
			}

			return (BOOL)GetSysColorBrush(COLOR_MENU);
		}
	
		return TRUE;
	case WM_DESTROY:
		Terminate = true;
		Sleep(500);
		DeleteObject(font);
		CloseHandle(hClock);
		PostQuitMessage(0);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return TRUE;

	}
	return FALSE;
}