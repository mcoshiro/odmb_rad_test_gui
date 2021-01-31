/**
 * Simple application to control KCU105 evaluation board during ODMB7/5 clock 
 * synthesizer and fanout chip SEU radiation test
 */

#ifndef UNICODE
#define UNICODE
#endif 

#include <cstdlib>
#include <new>
#include <sstream>
#include <string>

#include <windows.h>
#include <winuser.h>

const int DIAG_ID_STARTBUTTON = 0;
const int DIAG_ID_LOGTEXT = 1;
const int DIAG_ID_COMMENTBOX = 2;
const int DIAG_ID_LOADFANOUTFW = 3;
const int DIAG_ID_LOADCLOCKSYNTHFW = 4;
const int DIAG_ID_EXIT = 5;
const int DIAG_ID_WRITE_LOG = 6;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct StateInfo {
  int counter;
};

//useful tutorial: http://www.winprog.org/tutorial/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
  // Register the window class.
  const wchar_t CLASS_NAME[]  = L"ODMB Radiation Test Software Window Class";
  WNDCLASS wc = { };
  wc.lpfnWndProc   = WindowProc;
  wc.hInstance     = hInstance;
  wc.lpszClassName = CLASS_NAME;
  RegisterClass(&wc);
  StateInfo *AppState = new (std::nothrow) StateInfo;

  // Create the window.
  HWND hwnd = CreateWindowEx(
      0,                                // Optional window styles.
      CLASS_NAME,                       // Window class
      L"ODMB Radiation Test Software",  // Window text
      WS_OVERLAPPEDWINDOW,              // Window style
      // Size and position
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
      NULL,       // Parent window    
      NULL,       // Menu
      hInstance,  // Instance handle
      (LPVOID)AppState    // Additional application data
      );

  if (hwnd == NULL)
  {
    return 0;
  }
  ShowWindow(hwnd, nCmdShow);

  //Create elements in window
  //button to start test
  HWND hwndStartButton = CreateWindowEx(
      0,          // Optional Style
      L"BUTTON",  // Predefined class; Unicode assumed
      L"Start SEU Test",   // Button text
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
      16,         // x position
      368,         // y position
      128,         // Button width
      24,         // Button height
      hwnd,       // Parent window
      (HMENU)DIAG_ID_STARTBUTTON,          // Identifier for button
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
      NULL);      // Pointer not needed.

  //button to load firmware for test 1
  HWND hwndFwFanoutButton = CreateWindowEx(0, L"BUTTON", 
      L"Load FW (Fanout Test)", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 368, 192, 
      24, hwnd, (HMENU)DIAG_ID_LOADFANOUTFW, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to load firmware for test 2
  HWND hwndFwClockButton = CreateWindowEx(0, L"BUTTON", 
      L"Load FW (Clock Chip Test)", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 368, 368, 192, 
      24, hwnd, (HMENU)DIAG_ID_LOADCLOCKSYNTHFW, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to write log
  HWND hwndWriteLogButton = CreateWindowEx(0, L"BUTTON", 
      L"Write Log", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 16, 400, 128, 
      24, hwnd, (HMENU)DIAG_ID_WRITE_LOG, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to exit
  HWND hwndExitButton = CreateWindowEx(0, L"BUTTON", 
      L"Exit", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 400, 128, 
      24, hwnd, (HMENU)DIAG_ID_EXIT, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //text box
  HWND hwndCommentBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"test",
      WS_CHILD | WS_VISIBLE, 16, 432, 160, 32, hwnd, (HMENU)DIAG_ID_COMMENTBOX, 
      NULL, NULL);

  //text
  HWND hwndLogText = CreateWindowEx(0, L"STATIC", L"0", WS_CHILD | WS_VISIBLE, 
      16, 16, 544, 336, hwnd, (HMENU)DIAG_ID_LOGTEXT, NULL, NULL);

  // Run the message loop.
  MSG msg = { };
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_CREATE:
    {
      CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
      StateInfo* AppState = reinterpret_cast<StateInfo*>(pCreate->lpCreateParams);
      AppState->counter = 0;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)AppState);
    }
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
      EndPaint(hwnd, &ps);
    }
    return 0;

  case WM_COMMAND:
    if (HIWORD(wParam) == BN_CLICKED) {
      if (LOWORD(wParam) == DIAG_ID_STARTBUTTON) {
        //MessageBox(NULL, L"You clicked a button!", L"Button", MB_OK);
        //SendDlgItemMessage(hwnd, 1, 
        LONG_PTR AppStateLongPtr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        StateInfo* AppState = reinterpret_cast<StateInfo*>(AppStateLongPtr);
        AppState->counter += 1;
        std::wostringstream stros;
        stros << AppState->counter;
        SetDlgItemText(hwnd, DIAG_ID_LOGTEXT, stros.str().c_str());
      }
      if (LOWORD(wParam) == DIAG_ID_LOADFANOUTFW) {
        system("C:\\cygwin64\\bin\\python3.6m.exe test.py");
      }
    }
    return 0;

  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
