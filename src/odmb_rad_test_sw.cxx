/**
 * Simple application to control KCU105 evaluation board during ODMB7/5 clock 
 * synthesizer and fanout chip SEU radiation test
 */

#ifndef UNICODE
#define UNICODE
#endif 

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

//constants
const int DIAG_ID_STARTBUTTON = 0;
const int DIAG_ID_LOGTEXT = 1;
const int DIAG_ID_COMMENTBOX = 2;
const int DIAG_ID_LOADFANOUTFW = 3;
const int DIAG_ID_LOADCLOCKSYNTHFW = 4;
const int DIAG_ID_EXIT = 5;
const int DIAG_ID_WRITE_COMMENT = 6;
const int DIAG_ID_STOPBUTTON = 7;
const int DIAG_ID_INJECTERROR = 8;
const int DIAG_ID_LINK1TEXT = 9;
const int DIAG_ID_LINK2TEXT = 10;
const int DIAG_ID_RESETLINKS = 11;
const int DIAG_ID_RESETSEU = 12;
const int DIAG_ID_LINK1SEU = 13;
const int DIAG_ID_LINK2SEU = 14;
const int DIAG_ID_LINK1TITLE = 15;
const int DIAG_ID_LINK2TITLE = 16;
const int DIAG_ID_LINK1RATE = 17;
const int DIAG_ID_LINK2RATE = 18;
const int DIAG_ID_LINK1PLL = 19;
const int DIAG_ID_LINK2PLL = 20;
const int DIAG_ID_STARTTESTREAL = 21;
const int DIAG_ID_STOPTESTREAL = 22;

const int TIMER_ID_CHECKFILES = 0;

const int DISPLAY_LOG_LENGTH = 16;

//funcion forward declarations (TODO: move to header?)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void update_log(std::vector<std::wstring> & display_log, std::string new_text, std::ofstream *log_file, std::ofstream *test_log_file);
void update_log(std::vector<std::wstring> & display_log, std::wstring new_text, std::ofstream *log_file, std::ofstream *test_log_file);
void post_string_vector_to_dialog_text(HWND hwnd, int dialog_id, std::vector<std::wstring> & display_log);
std::string asctime_to_filetime(std::string asctime);
//std::wstring convert_to_log(std::string text);

enum class LinkStatus {
  test_stopped,
  link_connected,
  link_broken,
  communication_lost
};

struct StateInfo {
  LinkStatus link_one;
  LinkStatus link_two;
  int link1_seu_counter;
  int link2_seu_counter;
  int cycles_since_comm_counter;
  bool test_is_initiated; //indicates load FW button pressed
  bool test_is_running; //indicates FW loaded
  bool test_is_ready; //indicates start test button pressed
  std::vector<std::wstring> display_log;
  std::ofstream *log_file;
  std::ofstream *test_log_file;
};

//main function
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
      CW_USEDEFAULT, CW_USEDEFAULT, 784, 576,
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
      L"Launch Script",   // Button text
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
      16,         // x position
      336,         // y position
      128,         // Button width
      24,         // Button height
      hwnd,       // Parent window
      (HMENU)DIAG_ID_STARTBUTTON,          // Identifier for button
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
      NULL);      // Pointer not needed.

  ////button to load firmware for test 1
  //HWND hwndFwFanoutButton = CreateWindowEx(0, L"BUTTON", 
  //    L"Set FW (Fanout Test)", 
  //    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 400, 192, 
  //    24, hwnd, (HMENU)DIAG_ID_LOADFANOUTFW, 
  //    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  ////button to load firmware for test 2
  //HWND hwndFwClockButton = CreateWindowEx(0, L"BUTTON", 
  //    L"Set FW (Clock Chip Test)", 
  //    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 368, 400, 192, 
  //    24, hwnd, (HMENU)DIAG_ID_LOADCLOCKSYNTHFW, 
  //    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //text
  HWND hwndLogText = CreateWindowEx(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 
      16, 16, 720, 304, hwnd, (HMENU)DIAG_ID_LOGTEXT, NULL, NULL);

  //button to stop
  HWND hwndStopButton = CreateWindowEx(0, L"BUTTON", 
      L"Stop Script", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 336, 128, 
      24, hwnd, (HMENU)DIAG_ID_STOPBUTTON, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to exit
  HWND hwndExitButton = CreateWindowEx(0, L"BUTTON", 
      L"Exit", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 304, 336, 128, 
      24, hwnd, (HMENU)DIAG_ID_EXIT, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to write log
  HWND hwndWriteLogButton = CreateWindowEx(0, L"BUTTON", 
      L"Write Comment", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 16, 368, 128, 
      24, hwnd, (HMENU)DIAG_ID_WRITE_COMMENT, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to inject errors
  HWND hwndErrorButton = CreateWindowEx(0, L"BUTTON", 
      L"Inject Error", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 368, 128, 
      24, hwnd, (HMENU)DIAG_ID_INJECTERROR, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to reset links
  HWND hwndResetButton = CreateWindowEx(0, L"BUTTON", 
      L"Reset Links", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 304, 368, 128, 
      24, hwnd, (HMENU)DIAG_ID_RESETLINKS, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to reset SEU counter
  HWND hwndResetSEUButton = CreateWindowEx(0, L"BUTTON", 
      L"Reset SEUs", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 16, 400, 128, 
      24, hwnd, (HMENU)DIAG_ID_RESETSEU, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to start SEU test
  HWND hwndStartTestRealButton = CreateWindowEx(0, L"BUTTON", 
      L"Start SEU Test", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 400, 128, 
      24, hwnd, (HMENU)DIAG_ID_STARTTESTREAL, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //button to stop SEU test
  HWND hwndStopTestRealButton = CreateWindowEx(0, L"BUTTON", 
      L"Stop SEU Test", 
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 304, 400, 128, 
      24, hwnd, (HMENU)DIAG_ID_STOPTESTREAL, 
      (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);

  //text box
  HWND hwndCommentBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
      WS_CHILD | WS_VISIBLE, 16, 496, 720, 32, hwnd, (HMENU)DIAG_ID_COMMENTBOX, 
      NULL, NULL);

  //link title 1
  HWND hwndLinkOneTitleBox = CreateWindowEx(WS_EX_LEFT, L"STATIC", L"Link 0",
      WS_CHILD | WS_VISIBLE, 448, 336, 144, 24, hwnd, (HMENU)DIAG_ID_LINK1TITLE, 
      NULL, NULL);

  //link title 2
  HWND hwndLinkTwoTitleBox = CreateWindowEx(WS_EX_LEFT, L"STATIC", L"Link 1",
      WS_CHILD | WS_VISIBLE, 608, 336, 144, 24, hwnd, (HMENU)DIAG_ID_LINK2TITLE, 
      NULL, NULL);

  //link box 1
  HWND hwndLinkOneBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"Test Stopped",
      WS_CHILD | WS_VISIBLE, 448, 368, 144, 24, hwnd, (HMENU)DIAG_ID_LINK1TEXT, 
      NULL, NULL);

  //link box 2
  HWND hwndLinkTwoBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"Test Stopped",
      WS_CHILD | WS_VISIBLE, 608, 368, 144, 24, hwnd, (HMENU)DIAG_ID_LINK2TEXT, 
      NULL, NULL);

  //SEU box 1
  HWND hwndLinkOneSeuBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"0 SEUs",
      WS_CHILD | WS_VISIBLE, 448, 400, 144, 24, hwnd, (HMENU)DIAG_ID_LINK1SEU, 
      NULL, NULL);

  //SEU box 2
  HWND hwndLinkTwoSeuBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"0 SEUs",
      WS_CHILD | WS_VISIBLE, 608, 400, 144, 24, hwnd, (HMENU)DIAG_ID_LINK2SEU, 
      NULL, NULL);

  //link rate box 1
  HWND hwndLinkOneRateBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"0 Gb/s",
      WS_CHILD | WS_VISIBLE, 448, 432, 144, 24, hwnd, (HMENU)DIAG_ID_LINK1RATE, 
      NULL, NULL);

  //link rate box 2
  HWND hwndLinkTwoRateBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"0 Gb/s",
      WS_CHILD | WS_VISIBLE, 608, 432, 144, 24, hwnd, (HMENU)DIAG_ID_LINK2RATE, 
      NULL, NULL);

  //link PLL box 1
  HWND hwndLinkOnePllBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"Test Stopped",
      WS_CHILD | WS_VISIBLE, 448, 464, 144, 24, hwnd, (HMENU)DIAG_ID_LINK1PLL, 
      NULL, NULL);

  //link PLL box 2
  HWND hwndLinkTwoPllBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", L"Test Stopped",
      WS_CHILD | WS_VISIBLE, 608, 464, 144, 24, hwnd, (HMENU)DIAG_ID_LINK2PLL, 
      NULL, NULL);

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

  static HBRUSH link_one_bkgd;
  static HBRUSH link_two_bkgd;

  switch (uMsg)
  {
  case WM_CREATE:
    {
      //initialize AppState data and set pointer to allow access to it
      CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
      StateInfo* AppState = reinterpret_cast<StateInfo*>(pCreate->lpCreateParams);
      AppState->link1_seu_counter = 0;
      AppState->link2_seu_counter = 0;
      AppState->cycles_since_comm_counter = 0;
      AppState->test_is_initiated = false;
      AppState->test_is_running = false;
      AppState->test_is_ready = false;
      AppState->link_one = LinkStatus::test_stopped;
      AppState->link_two = LinkStatus::test_stopped;
      AppState->display_log = std::vector<std::wstring>(DISPLAY_LOG_LENGTH,L"");
      std::time_t current_time = std::time(nullptr);
      char* time_cstr = std::asctime(std::localtime(&current_time));
      std::string time_string = asctime_to_filetime(std::string(time_cstr));
      AppState->log_file = new std::ofstream(("logs\\radsession_"+time_string+".log").c_str());
      AppState->test_log_file = nullptr;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)AppState);

      //initialize static variables in WindowProc
      link_one_bkgd = CreateSolidBrush(RGB(160,160,160));
      link_two_bkgd = CreateSolidBrush(RGB(160,160,160));

      //delete any comm files left over from previous runs
      std::ifstream tcl_comm_file("comm_files\\tcl_comm_out.txt");
      if (tcl_comm_file.good()) {
        tcl_comm_file.close();
        remove("comm_files\\tcl_comm_out.txt");
      }
      tcl_comm_file = std::ifstream("comm_files\\tcl_comm_in.txt");
      if (tcl_comm_file.good()) {
        tcl_comm_file.close();
        remove("comm_files\\tcl_comm_in.txt");
      }

      //start 5s timer on which program reads directory files for Vivado output
      //note it takes O(0.5) seconds just to find if a file exists
      SetTimer(hwnd, TIMER_ID_CHECKFILES, 5000, (TIMERPROC)NULL);
    }
    return 0;

  case WM_DESTROY:
    {
      LONG_PTR AppStateLongPtr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
      StateInfo* AppState = reinterpret_cast<StateInfo*>(AppStateLongPtr);
      //quit Vivado if test still running
      if (AppState->test_is_running) {
        std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
        tcl_comm_file << "cmd: stop\n";
        AppState->test_is_running = false;
        tcl_comm_file.close();
      }
      //close log file
      AppState->log_file->close();
      delete AppState->log_file;
      if (AppState->test_log_file != nullptr) {
        AppState->test_log_file->close();
        delete AppState->test_log_file;
      }
      DeleteObject( link_one_bkgd );
      DeleteObject( link_two_bkgd );
      PostQuitMessage(0);
    }
    return 0;

  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
      EndPaint(hwnd, &ps);
    }
    return 0;

  case WM_CTLCOLORSTATIC:
    {
      LONG_PTR AppStateLongPtr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
      StateInfo* AppState = reinterpret_cast<StateInfo*>(AppStateLongPtr);
      HDC hdcStatic = reinterpret_cast<HDC>(wParam);

      int dialog_id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));

      if (dialog_id == DIAG_ID_LINK1TEXT) {
        switch (AppState->link_one) 
        {
        case LinkStatus::link_connected:
          SetBkColor(hdcStatic, RGB(51,255,51));
          break;

        case LinkStatus::link_broken:
          SetBkColor(hdcStatic, RGB(255,51,51));
          break;

        case LinkStatus::communication_lost:
          SetBkColor(hdcStatic, RGB(255,153,51));
          break;

        default:
          //test stopped
          SetBkColor(hdcStatic, RGB(160,160,160));
        }
      }
      else if (dialog_id == DIAG_ID_LINK2TEXT) {
        switch (AppState->link_two) 
        {
        case LinkStatus::link_connected:
          SetBkColor(hdcStatic, RGB(51,255,51));
          break;

        case LinkStatus::link_broken:
          SetBkColor(hdcStatic, RGB(255,51,51));
          break;

        case LinkStatus::communication_lost:
          SetBkColor(hdcStatic, RGB(255,153,51));
          break;

        default:
          //test stopped
          SetBkColor(hdcStatic, RGB(160,160,160));
        }
      }

      if (dialog_id == DIAG_ID_LINK1TEXT) {
        return (INT_PTR)link_one_bkgd;
      }
      else if (dialog_id == DIAG_ID_LINK2TEXT) {
        return (INT_PTR)link_two_bkgd;
      }
    }
    return 0;

  case WM_TIMER:
    {
      switch (wParam)
      {
      case TIMER_ID_CHECKFILES:
        {
          LONG_PTR AppStateLongPtr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
          StateInfo* AppState = reinterpret_cast<StateInfo*>(AppStateLongPtr);
          //check if Vivado has left us any log information
          std::ifstream tcl_comm_file("comm_files\\tcl_comm_out.txt");
          if (tcl_comm_file.good()) {
            AppState->cycles_since_comm_counter = 0;
            std::string line;
            while (getline(tcl_comm_file, line)) {
              if (line.size() >= 5) {
                if (line.substr(0,5) == "log: ") {
                  //add info to log
                  std::string log_line = line.substr(5,line.size()-5);
                  update_log(AppState->display_log, log_line, AppState->log_file, AppState->test_log_file);
                }
              }
              if (line.size() >= 18) {
                if (line.substr(0,18) == "sync: test started") {
                  AppState->test_is_running = true;
                }
              }
              if (line.size() >= 18) {
                if (line.substr(0,18) == "sync: test stopped") {
                  AppState->test_is_running = false;
                  AppState->link_one = LinkStatus::test_stopped;
                  AppState->link_two = LinkStatus::test_stopped;
                  DeleteObject( link_one_bkgd );
                  DeleteObject( link_two_bkgd );
                  link_one_bkgd = CreateSolidBrush(RGB(160,160,160));
                  link_two_bkgd = CreateSolidBrush(RGB(160,160,160));
                  SetDlgItemText(hwnd, DIAG_ID_LINK1TEXT, L"Test Stopped");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2TEXT, L"Test Stopped");
                  SetDlgItemText(hwnd, DIAG_ID_LINK1RATE, L"0 Gb/s");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2RATE, L"0 Gb/s");
                  SetDlgItemText(hwnd, DIAG_ID_LINK1PLL, L"Test Stopped");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2PLL, L"Test Stopped");
                }
              }
              if (line.size() >= 17) {
                if (line.substr(0,17) == "sync: test paused") {
                  AppState->test_is_ready = false;
                  AppState->link_one = LinkStatus::test_stopped;
                  AppState->link_two = LinkStatus::test_stopped;
                  DeleteObject( link_one_bkgd );
                  DeleteObject( link_two_bkgd );
                  link_one_bkgd = CreateSolidBrush(RGB(160,160,160));
                  link_two_bkgd = CreateSolidBrush(RGB(160,160,160));
                  SetDlgItemText(hwnd, DIAG_ID_LINK1TEXT, L"Test Stopped");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2TEXT, L"Test Stopped");
                  SetDlgItemText(hwnd, DIAG_ID_LINK1RATE, L"0 Gb/s");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2RATE, L"0 Gb/s");
                  SetDlgItemText(hwnd, DIAG_ID_LINK1PLL, L"Test Stopped");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2PLL, L"Test Stopped");
                }
              }
              if (line.size() >= 19) {
                if (line.substr(0,19) == "sync: link 0 broken") {
                  AppState->link_one = LinkStatus::link_broken;
                  DeleteObject( link_one_bkgd );
                  link_one_bkgd = CreateSolidBrush(RGB(255,51,51));
                  SetDlgItemText(hwnd, DIAG_ID_LINK1TEXT, L"Link Broken");
                  SetDlgItemText(hwnd, DIAG_ID_LINK1RATE, L"0 Gb/s");
                }
              }
              if (line.size() >= 22) {
                if (line.substr(0,22) == "sync: link 0 connected") {
                  AppState->link_one = LinkStatus::link_connected;
                  DeleteObject( link_one_bkgd );
                  link_one_bkgd = CreateSolidBrush(RGB(51,255,51));
                  SetDlgItemText(hwnd, DIAG_ID_LINK1TEXT, L"Link Connected");
                }
              }
              if (line.size() >= 19) {
                if (line.substr(0,19) == "sync: link 1 broken") {
                  AppState->link_two = LinkStatus::link_broken;
                  DeleteObject( link_two_bkgd );
                  link_two_bkgd = CreateSolidBrush(RGB(255,51,51));
                  SetDlgItemText(hwnd, DIAG_ID_LINK2TEXT, L"Link Broken");
                  SetDlgItemText(hwnd, DIAG_ID_LINK2RATE, L"0 Gb/s");
                }
              }
              if (line.size() >= 22) {
                if (line.substr(0,22) == "sync: link 1 connected") {
                  AppState->link_two = LinkStatus::link_connected;
                  DeleteObject( link_two_bkgd );
                  link_two_bkgd = CreateSolidBrush(RGB(51,255,51));
                  SetDlgItemText(hwnd, DIAG_ID_LINK2TEXT, L"Link Connected");
                }
              }
              if (line.size() >= 22) {
                if (line.substr(0,22) == "sync: link 0 SEU count") {
                  std::string str_seu_count = line.substr(22,line.size()) + " SEUs";
                  std::wstring wstr_seu_count = std::wstring(str_seu_count.begin(), str_seu_count.end());
                  std::wostringstream stros;
                  stros << wstr_seu_count;
                  SetDlgItemText(hwnd, DIAG_ID_LINK1SEU, stros.str().c_str());
                }
              }
              if (line.size() >= 22) {
                if (line.substr(0,22) == "sync: link 1 SEU count") {
                  std::string str_seu_count = line.substr(22,line.size()) + " SEUs";
                  std::wstring wstr_seu_count = std::wstring(str_seu_count.begin(), str_seu_count.end());
                  std::wostringstream stros;
                  stros << wstr_seu_count;
                  SetDlgItemText(hwnd, DIAG_ID_LINK2SEU, stros.str().c_str());
                }
              }
              if (line.size() >= 20) {
                if (line.substr(0,20) == "sync: link 0 status:") {
                  std::string str_seu_count = line.substr(20,line.size()) + " Gb/s";
                  std::wstring wstr_seu_count = std::wstring(str_seu_count.begin(), str_seu_count.end());
                  std::wostringstream stros;
                  stros << wstr_seu_count;
                  SetDlgItemText(hwnd, DIAG_ID_LINK1RATE, stros.str().c_str());
                }
              }
              if (line.size() >= 20) {
                if (line.substr(0,20) == "sync: link 1 status:") {
                  std::string str_seu_count = line.substr(20,line.size()) + " Gb/s";
                  std::wstring wstr_seu_count = std::wstring(str_seu_count.begin(), str_seu_count.end());
                  std::wostringstream stros;
                  stros << wstr_seu_count;
                  SetDlgItemText(hwnd, DIAG_ID_LINK2RATE, stros.str().c_str());
                }
              }
              if (line.size() >= 16) {
                if (line.substr(0,16) == "sync: link 0 PLL") {
                  if (line.substr(17,1) == "l") 
                    SetDlgItemText(hwnd, DIAG_ID_LINK1PLL, L"PLL Locked");
                  else
                    SetDlgItemText(hwnd, DIAG_ID_LINK1PLL, L"PLL Unlocked");
                }
              }
              if (line.size() >= 16) {
                if (line.substr(0,16) == "sync: link 1 PLL") {
                  if (line.substr(17,1) == "l") 
                    SetDlgItemText(hwnd, DIAG_ID_LINK2PLL, L"PLL Locked");
                  else
                    SetDlgItemText(hwnd, DIAG_ID_LINK2PLL, L"PLL Unlocked");
                }
              }
            }
            //write to box, delete communication file
            post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            tcl_comm_file.close();
            remove("comm_files\\tcl_comm_out.txt");
          }
          else if (AppState->test_is_ready == true) {
            AppState->cycles_since_comm_counter += 1;
          }
          if (AppState->cycles_since_comm_counter == 4) {
            //set link status to communication lost after 20 s
            AppState->link_one = LinkStatus::communication_lost;
            AppState->link_two = LinkStatus::communication_lost;
            DeleteObject( link_one_bkgd );
            DeleteObject( link_two_bkgd );
            link_one_bkgd = CreateSolidBrush(RGB(255,153,51));
            link_two_bkgd = CreateSolidBrush(RGB(255,153,51));
            SetDlgItemText(hwnd, DIAG_ID_LINK1TEXT, L"Communication Lost");
            SetDlgItemText(hwnd, DIAG_ID_LINK2TEXT, L"Communication Lost");
          }
        }
        break;

      default: //nothing
        break;
      }
    }
    return 0;

  case WM_COMMAND:
    {
      if (HIWORD(wParam) == BN_CLICKED) {
        LONG_PTR AppStateLongPtr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        StateInfo* AppState = reinterpret_cast<StateInfo*>(AppStateLongPtr);

        switch (LOWORD(wParam))
        {
        case DIAG_ID_STARTBUTTON:
          //MessageBox(NULL, L"You clicked a button!", L"Button", MB_OK);
          if (AppState->test_is_initiated) {
            //warning, test already running
            update_log(AppState->display_log, 
                std::wstring(L"Error: cannot start test since test already initiated"), 
                AppState->log_file, AppState->test_log_file);
            post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
          }
          else {
            //system("start C:\\cygwin64\\bin\\python3.6m.exe scripts\\dummy_python_script.py && exit");
            //system("start C:\\Xilinx\\Vivado\\2019.2\\bin\\vivado -nojournal -nolog -mode batch -notrace -source scripts\\run_seu_test.tcl && exit");
            system("start scripts\\run_vivado.bat");
            AppState->test_is_initiated = true;
            update_log(AppState->display_log, std::wstring(L"Starting script and loading firmware, please wait"), AppState->log_file, AppState->test_log_file);
            post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            //Keep GUI window on top: TODO doesn't work, not important enough to fix now
            RECT win_rect;
            GetWindowRect(hwnd, &win_rect);
            SetWindowPos(hwnd, HWND_TOP, win_rect.left, win_rect.top, 576, 480, SWP_NOSIZE);
            //reset SEU counters
            SetDlgItemText(hwnd, DIAG_ID_LINK1SEU, L"0 SEUs");
            SetDlgItemText(hwnd, DIAG_ID_LINK2SEU, L"0 SEUs");
          }
          break;

        case DIAG_ID_STOPBUTTON:
          {
            if (!AppState->test_is_running) {
              //warning, no running test
              update_log(AppState->display_log, 
                  std::wstring(L"Warning: attempting to stop script, but no known running process"), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            std::ifstream tcl_comm_check_file("comm_files\\tcl_comm_in.txt");
            if (tcl_comm_check_file.good()) {
              //error, unread communication to Vivado already exists
              update_log(AppState->display_log, 
                  std::wstring(L"Error: attempted to stop script but previous command unresponsive"), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            else {
              std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
              tcl_comm_file << "cmd: stop\n";
              tcl_comm_file.close();
            }
            AppState->test_is_initiated = false;
            AppState->test_is_running = false;
            AppState->test_is_ready = false;
            if (AppState->test_log_file != nullptr) {
              AppState->test_log_file->close();
              delete AppState->test_log_file;
              AppState->test_log_file = nullptr;
            }
          }
          break;

        case DIAG_ID_INJECTERROR:
          {
            if (!AppState->test_is_ready) {
              //warning, no running test
              update_log(AppState->display_log, 
                  std::wstring(L"Error: no running test to inject errors"), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            else {
              std::ifstream tcl_comm_check_file("comm_files\\tcl_comm_in.txt");
              if (tcl_comm_check_file.good()) {
                //error, unread communication to Vivado already exists
                update_log(AppState->display_log, 
                    std::wstring(L"Error: attempted to inject error but previous command unresponsive"), 
                    AppState->log_file, AppState->test_log_file);
                post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
              }
              else {
                std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
                tcl_comm_file << "cmd: inject error\n";
                tcl_comm_file.close();
              }
            }
          }
          break;

        case DIAG_ID_RESETLINKS:
          {
            if (!AppState->test_is_ready) {
              //warning, no running test
              update_log(AppState->display_log, 
                  std::wstring(L"Error: no running test to reset links"), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            else {
            std::ifstream tcl_comm_check_file("comm_files\\tcl_comm_in.txt");
              if (tcl_comm_check_file.good()) {
                //error, unread communication to Vivado already exists
                update_log(AppState->display_log, 
                    std::wstring(L"Error: attempted to reset links but previous command unresponsive"), 
                    AppState->log_file, AppState->test_log_file);
                post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
              }
              else {
                update_log(AppState->display_log, 
                    std::wstring(L"User reset links"), 
                    AppState->log_file, AppState->test_log_file);
                std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
                tcl_comm_file << "cmd: reset links\n";
                tcl_comm_file.close();
              }
            }
          }
          break;

        case DIAG_ID_RESETSEU:
          {
            if (!AppState->test_is_ready) {
              //warning, no running test
              update_log(AppState->display_log, 
                  std::wstring(L"Error: no running test to reset SEUs"), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            else {
            std::ifstream tcl_comm_check_file("comm_files\\tcl_comm_in.txt");
              if (tcl_comm_check_file.good()) {
                //error, unread communication to Vivado already exists
                update_log(AppState->display_log, 
                    std::wstring(L"Error: attempted to reset SEUs but previous command unresponsive"), 
                    AppState->log_file, AppState->test_log_file);
                post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
              }
              else {
                update_log(AppState->display_log, 
                    std::wstring(L"User reset SEUs"), 
                    AppState->log_file, AppState->test_log_file);
                std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
                tcl_comm_file << "cmd: reset seus\n";
                tcl_comm_file.close();
                SetDlgItemText(hwnd, DIAG_ID_LINK1SEU, L"0 SEUs");
                SetDlgItemText(hwnd, DIAG_ID_LINK2SEU, L"0 SEUs");
              }
            }
          }
          break;

        case DIAG_ID_STARTTESTREAL:
          {
            if (!AppState->test_is_running) {
              //warning, no running test
              update_log(AppState->display_log, 
                  std::wstring(L"Error: please load FW before starting test."), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            else {
            std::ifstream tcl_comm_check_file("comm_files\\tcl_comm_in.txt");
              if (tcl_comm_check_file.good()) {
                //error, unread communication to Vivado already exists
                update_log(AppState->display_log, 
                    std::wstring(L"Error: attempted to start test but previous command unresponsive."), 
                    AppState->log_file, AppState->test_log_file);
                post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
              }
              else {
                std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
                tcl_comm_file << "cmd: reset links\n";
                tcl_comm_file << "cmd: reset seus\n";
                tcl_comm_file << "cmd: start\n";
                tcl_comm_file.close();
                std::time_t current_time = std::time(nullptr);
                char* time_cstr = std::asctime(std::localtime(&current_time));
                std::string time_string = asctime_to_filetime(std::string(time_cstr));
                AppState->test_log_file = new std::ofstream(("logs\\radtest_"+time_string+".log").c_str());
                SetDlgItemText(hwnd, DIAG_ID_LINK1SEU, L"0 SEUs");
                SetDlgItemText(hwnd, DIAG_ID_LINK2SEU, L"0 SEUs");
                AppState->test_is_ready = true;
                update_log(AppState->display_log, 
                    std::wstring(L"Starting test"), 
                    AppState->log_file, AppState->test_log_file);
              }
            }
          }
          break;

        case DIAG_ID_STOPTESTREAL:
          {
            if (!AppState->test_is_ready) {
              //warning, no running test
              update_log(AppState->display_log, 
                  std::wstring(L"Error: no running test to stop"), 
                  AppState->log_file, AppState->test_log_file);
              post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            }
            else {
            std::ifstream tcl_comm_check_file("comm_files\\tcl_comm_in.txt");
              if (tcl_comm_check_file.good()) {
                //error, unread communication to Vivado already exists
                update_log(AppState->display_log, 
                    std::wstring(L"Error: attempted to stop test but previous command unresponsive."), 
                    AppState->log_file, AppState->test_log_file);
                post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
              }
              else {
                update_log(AppState->display_log, 
                    std::wstring(L"Stopping test"), 
                    AppState->log_file, AppState->test_log_file);
                std::ofstream tcl_comm_file("comm_files\\tcl_comm_in.txt");
                tcl_comm_file << "cmd: pause\n";
                tcl_comm_file.close();
                AppState->test_is_ready = false;
                if (AppState->test_log_file != nullptr) {
                  AppState->test_log_file->close();
                  delete AppState->test_log_file;
                  AppState->test_log_file = nullptr;
                }
                else {
                  std::cout << "ERROR: test was ready but test_log_file was nullptr";
                }
              }
            }
          }
          break;

        case DIAG_ID_WRITE_COMMENT:
          {
            wchar_t diag_comment_text[256];
            GetDlgItemText(hwnd, DIAG_ID_COMMENTBOX, diag_comment_text, 256);
            update_log(AppState->display_log, std::wstring(diag_comment_text), AppState->log_file, AppState->test_log_file);
            post_string_vector_to_dialog_text(hwnd, DIAG_ID_LOGTEXT, AppState->display_log);
            SetDlgItemText(hwnd, DIAG_ID_COMMENTBOX, L"");
          }
          break;
        
        case DIAG_ID_EXIT:
          //PostQuitMessage(0);
          DestroyWindow(hwnd);
          break;

        default: //nothing
          break;
        }
      }
    }
    return 0;

  default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);

  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * function that converts a time in std::asctime format to something that can
 * be written as a fulename
 */
std::string asctime_to_filetime(std::string asctime) {
  std::string month_char_string = asctime.substr(4,3);
  std::string month_num_string;
  if (month_char_string == "Jan") month_num_string = "01";
  else if (month_char_string == "Feb") month_num_string = "02";
  else if (month_char_string == "Mar") month_num_string = "03";
  else if (month_char_string == "Apr") month_num_string = "04";
  else if (month_char_string == "May") month_num_string = "05";
  else if (month_char_string == "Jun") month_num_string = "06";
  else if (month_char_string == "Jul") month_num_string = "07";
  else if (month_char_string == "Aug") month_num_string = "08";
  else if (month_char_string == "Sep") month_num_string = "09";
  else if (month_char_string == "Oct") month_num_string = "10";
  else if (month_char_string == "Nov") month_num_string = "11";
  else month_num_string = "12";
  std::string iso_form_time = asctime.substr(20,4); //year
  iso_form_time = iso_form_time + month_num_string; //month
  iso_form_time = iso_form_time + asctime.substr(8,2); //day
  iso_form_time = iso_form_time + "T";
  iso_form_time = iso_form_time + asctime.substr(11,2); //hour
  iso_form_time = iso_form_time + asctime.substr(14,2); //minute
  iso_form_time = iso_form_time + asctime.substr(17,2); //second
  return iso_form_time;
}

/**
 * function to display a string vector in a dialog box
 */
void post_string_vector_to_dialog_text(HWND hwnd, int dialog_id, 
                                       std::vector<std::wstring> & display_log) {
  std::wostringstream stros;
  for (unsigned log_idx = 0; log_idx < DISPLAY_LOG_LENGTH; log_idx++) {
    stros << display_log[log_idx] << "\n";
  }
  SetDlgItemText(hwnd, dialog_id, stros.str().c_str());
}

/**
 * function that updates vector display_log with new time-stampped text from new_text
 */
void update_log(std::vector<std::wstring> & display_log, std::string new_text, std::ofstream *log_file, std::ofstream *test_log_file) {
  //update log with new comment
  for (unsigned log_idx = 0; log_idx < (DISPLAY_LOG_LENGTH-1); log_idx++) {
    display_log[log_idx] = display_log[log_idx+1];
  }
  std::time_t current_time = std::time(nullptr);
  char* time_cstr = std::asctime(std::localtime(&current_time));
  std::string time_string = (std::string(time_cstr)).substr(4,20);
  //convert from std::string to std::wstring assuming all characters are single-byte
  std::string new_log = time_string + ": " + new_text;
  display_log[DISPLAY_LOG_LENGTH-1] = std::wstring(new_log.begin(), new_log.end());
  (*log_file) << new_log << "\n";
  if (test_log_file != nullptr) {
    (*test_log_file) << new_log << "\n";
  }
}

/**
 * function that updates vector display_log with new time-stampped text from new_text
 */
void update_log(std::vector<std::wstring> & display_log, std::wstring new_text, std::ofstream *log_file, std::ofstream *test_log_file) {
  //update log with new comment
  for (unsigned log_idx = 0; log_idx < (DISPLAY_LOG_LENGTH-1); log_idx++) {
    display_log[log_idx] = display_log[log_idx+1];
  }
  wchar_t diag_comment_text[256];
  std::time_t current_time = std::time(nullptr);
  char* time_cstr = std::asctime(std::localtime(&current_time));
  std::string time_string = (std::string(time_cstr)).substr(4,20);
  //convert from std::string to std::wstring assuming all characters are single-byte
  display_log[DISPLAY_LOG_LENGTH-1] = 
      std::wstring(time_string.begin(), time_string.end()) + L": " + new_text;
  (*log_file) << std::string(display_log[DISPLAY_LOG_LENGTH-1].begin(), 
      display_log[DISPLAY_LOG_LENGTH-1].end()) << "\n";
  if (test_log_file != nullptr) {
    (*test_log_file) << std::string(display_log[DISPLAY_LOG_LENGTH-1].begin(), 
        display_log[DISPLAY_LOG_LENGTH-1].end()) << "\n";
  }
}
