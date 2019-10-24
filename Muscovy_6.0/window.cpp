#include <Windows.h>
#include <Windowsx.h>
#include <Wincodec.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <queue>
#include <ctime>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "pqastar.h"
#include "timer.h"
#include "Sound.h"
#include "game.h"
#include "Map.h"
#include "Camera.h"
#include "Move.h"
#include "WalkEngine.h"
#include "Explore.h"
#include "character.h"
#include "Graphics.h"
#include "Battle.h"
#include "Animation.h"
#include "core.h"
#include "resource.h"


void Core::initialWindow() {
	static char szAppName[] = "Game";
	WNDCLASSEX  wndclass ;
	RECT rect = {0, 0, ResX-1, ResY+168-1};//1024x768   600+168 for the menu
	AdjustWindowRect(&rect, WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX, false);
	wndclass.cbSize        = sizeof (wndclass) ;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc   = RetriveWndProcPointer;
	wndclass.cbClsExtra    = 0 ;
	wndclass.cbWndExtra    = sizeof (Core*);//send Core::this pointer to LPCREATESTRUCT struct
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (nullptr, MAKEINTRESOURCE(IDI_ICON1));
	wndclass.hCursor = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)) ;
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
	wndclass.lpszMenuName  = nullptr;
	wndclass.lpszClassName = szAppName;
	wndclass.hIconSm       = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON2));//need load to hInstance
	RegisterClassEx (&wndclass) ;
	hwnd = CreateWindow (szAppName,
		"CS428/528 RPG Game Clockwork Muscovy 2D version Ver 6.0", // Window caption
		WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX,
		200,20,
		1030,788,/*rect.right-rect.left, rect.bottom-rect.top,*/
		nullptr, nullptr, hInstance, this) ;//last parameter must be this(Core::this pointer), send Core::this pointer to LPCREATESTRUCT struct
	if (hwnd == nullptr)
	{
		MessageBox(0, "CreateWindow Failed", 0, 0);
		//return false;
	}
	ShowWindow (hwnd, iCmdShow) ;
	UpdateWindow (hwnd) ;
}

LRESULT CALLBACK Core::RetriveWndProcPointer(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	if (iMsg == WM_NCCREATE) {
		//retrieve "this" pointer which has been passed by CreateWindow() last parameter: "this" pointer is stored in the first member of CREATESTRUCT lpCreateParams
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams));//retrieve Core this pointer
		return reinterpret_cast<Core*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams)->WndProc(hwnd, iMsg, wParam, lParam);
	} else {
		Core* curretnThis=reinterpret_cast<Core*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (!curretnThis) {
			return DefWindowProc(hwnd, iMsg, wParam, lParam);
		} else {
			return curretnThis->WndProc(hwnd, iMsg, wParam, lParam);
		}
	}
}
LRESULT CALLBACK Core::RetriveChildWndProcPointer(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	if (iMsg == WM_NCCREATE)	{
		//retrieve "this" pointer which has been passed by CreateWindow() last parameter: "this" pointer is stored in the first member of CREATESTRUCT lpCreateParams
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams));
		return reinterpret_cast<Core*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams)->ChildWndProc(hwnd, iMsg, wParam, lParam);
	} else {
		Core* curretnThis=reinterpret_cast<Core*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (!curretnThis) {
			return DefWindowProc(hwnd, iMsg, wParam, lParam);
		} else {
			return curretnThis->ChildWndProc(hwnd, iMsg, wParam, lParam);
		}
	}
}
//*********************************
//        Windows Procedure
// ********************************
LRESULT Core::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
		case WM_ACTIVATE: {
			OnMainActivate(wParam);
			return 0;
		}
		case WM_CREATE:	{
			OnMainCreate(hwnd);
			return 0 ;
		}
		case WM_PAINT:	{
			OnMainPaint(hwnd);
			return 0 ;
		}
		case WM_KEYDOWN: {
			OnKeyDown(wParam, lParam);
			return 0;
		}
		case WM_MOUSEWHEEL: {
			OnMouseWheel(wParam, lParam);
			return 0;
		}
		case	WM_LBUTTONDOWN: {
			OnMainLBUTTONDOWN(hwnd);
			return 0;
		}

		case WM_COMMAND: {
			OnMainCommand(wParam, lParam);
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}


//**********************************
//    child windows procedure
//**********************************
LRESULT  Core::ChildWndProc (HWND hChild, UINT iMsg, WPARAM wParam,LPARAM lParam) 
{
	HDC childDc;
	switch (iMsg) {
	case WM_CREATE: {
		childDc = GetDC(hChild);
		SelectObject(childDc, GetStockObject(SYSTEM_FIXED_FONT));
		return 0;
	}
	case WM_KEYDOWN: {
		OnKeyDown(wParam, lParam);
		return 0;
	}
	case WM_LBUTTONUP: {
		for (int i = 0; i < 4; ++i) {
			static_cast<Player*>(sprites[i])->getWalkEngine()->hold = false;
		}
		return 0;
	}
	case WM_MOUSEMOVE: {
		OnChildMouseMove(hChild, lParam);
		return 0;
	}
	case WM_MOUSEWHEEL: {
		OnMouseWheel(wParam, lParam);
		return 0;
	}
	case WM_LBUTTONDOWN: {
		OnChildLBUTTONDOWN(hChild, wParam, lParam);
		return 0;
	}
	case WM_PAINT: {
		childDc = GetDC(hChild);
		OnChildPaint(hChild, childDc);
		return 0;
	}
	case WM_DESTROY:
		return 0;
	}
	return DefWindowProc (hChild, iMsg, wParam, lParam) ;
}

LRESULT Core::OnMainCreate(HWND hwnd) {
	HDC hdc = GetDC (hwnd) ;
	// The system monospaced font is selected
	SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT)) ;
	hwndAttckBT=CreateWindow("BUTTON",
		"ATTACK",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		300,600+30,
		90,40,
		hwnd,
		(HMENU)ATTACKBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndAttckBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[2]); 
	SetClassLongPtr(hwndAttckBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));//-12 is GCL_HCURSOR
	hwndMoveBT=CreateWindow("BUTTON",
		"Move",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		300,600+95,
		90,40,
		hwnd,
		(HMENU)MOVEBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndMoveBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[3]); 
	SetClassLongPtr(hwndMoveBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	hwndUseItemBT=CreateWindow("BUTTON",
		"UseItem",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		750,600+30,
		150,30,
		hwnd,
		(HMENU)USEITEMBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndUseItemBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[5]); 
	SetClassLongPtr(hwndUseItemBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	hwndItemListBox=CreateWindow("COMBOBOX",
		"Item",
		//WS_CHILD|LBS_STANDARD/*LBS_NOTIFY|WS_VSCROLL*/,
		CBS_DROPDOWNLIST | WS_VSCROLL| WS_CHILD,
		750,600+100,
		150,150,
		hwnd,
		(HMENU)ITEMLISTBOX,
		hInstance,
		nullptr);
	ShowWindow(hwndItemListBox,SW_SHOW); 
	SetClassLongPtr(hwndItemListBox, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	hwndUseSkillBT=CreateWindow("BUTTON",
		"UseSkill",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		630,600+95,
		150,30,
		hwnd,
		(HMENU)USEITEMBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndUseSkillBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[6]); 
	SetClassLongPtr(hwndUseSkillBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	hwndSkillDropBox=CreateWindow("COMBOBOX",
		"Skill",
		//WS_CHILD|LBS_STANDARD/*LBS_NOTIFY|WS_VSCROLL*/,
		CBS_DROPDOWNLIST | WS_VSCROLL| WS_CHILD,
		630,730,
		150,150,
		hwnd,
		(HMENU)SKILLDROPBOX,
		hInstance,
		nullptr);
	//ShowWindow(hwndSkillDropBox,SW_SHOW);
	SetClassLongPtr(hwndSkillDropBox, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	hwndJornalBT=CreateWindow("BUTTON",
		"Jornal",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		300,600+30,
		90,40,
		hwnd,
		(HMENU)JOURNALBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndJornalBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[4]); 
	ShowWindow(hwndJornalBT,SW_SHOW);
	SetClassLongPtr(hwndJornalBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	hwndMapBT=CreateWindow("BUTTON",
		"Map",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		300,600+95,
		90,40,
		hwnd,
		(HMENU)MAPBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndMapBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[7]); 
	ShowWindow(hwndMapBT,SW_SHOW);
	SetClassLongPtr(hwndMapBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));

	hwndStatusBT=CreateWindow("BUTTON",
		"Status",
		WS_CHILD | BS_BITMAP | BS_PUSHBUTTON,
		630,600+30,
		90,40,
		hwnd,
		(HMENU)STATUSBUTTON,
		hInstance,
		nullptr);
	SendMessage(hwndStatusBT,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hbitmap[8]); 
	ShowWindow(hwndStatusBT,SW_SHOW);
	SetClassLongPtr(hwndStatusBT, -12, (LONG_PTR)LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)));
	//		    Create a child window
	WNDCLASSEX  chiclass ;
	chiclass.cbSize        = sizeof (chiclass) ;
	chiclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	chiclass.lpfnWndProc   = RetriveChildWndProcPointer;
	chiclass.cbClsExtra    = 0 ;
	chiclass.cbWndExtra    = sizeof(Core*) ;
	chiclass.hInstance     = hInstance ;
	chiclass.hIcon         = nullptr;
	chiclass.hCursor       = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_CURSOR1)) ;
	chiclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
	chiclass.lpszMenuName  = nullptr;
	chiclass.lpszClassName = "ChildWindow" ;
	chiclass.hIconSm       = nullptr;

	RegisterClassEx (&chiclass) ;
	hChild = CreateWindow ("ChildWindow",
		"A Child Window",  // caption
		WS_CHILD ,
		//WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW ,
		0, 0,          // x and y of window location
		ResX, ResY,        // x and y of window size
		hwnd,            // handle to the parent window 
		(HMENU) 1001,    // child window designation
		hInstance,       // program instance
		this) ;
	return 0;
}
LRESULT Core::OnMainActivate(WPARAM wParam) {
	if (LOWORD(wParam) == WA_INACTIVE) {
		gameTimer.Pause();
	} else {
		gameTimer.Resume();
	}
	return 0;
}
LRESULT Core::OnMainPaint(HWND hwnd) {
	PAINTSTRUCT ps;
	HDC hdc=BeginPaint (hwnd, &ps) ;
	if (hChild!=nullptr) {
		UpdateWindow(hChild);
		ShowWindow(hChild, SW_SHOWNORMAL);
	}
	if (mode == 1 || mode == 0) {//intro mode or exploring mode: use map
		HDC hdcBanner = CreateCompatibleDC(hdc);
		HBITMAP hbmOldHorizontal = (HBITMAP)SelectObject(hdcBanner,hbitmap[1]); //hbitmap[1]=".\\HUD\\CharBar1024Item.bmp" with inventory fake button lower screen background
		StretchBlt(hdc,0,600,1024,160,hdcBanner,0,0,1021,160,SRCCOPY);
		SelectObject(hdcBanner,hbmOldHorizontal);
		DeleteDC(hdcBanner);
	} else if(mode == 4) {//dialogue mode, use picture global variable dynamically determins the dialogue picture
		HDC hdcBanner = CreateCompatibleDC(hdc);
		HBITMAP hbmOldHorizontal = (HBITMAP)SelectObject(hdcBanner,picture); 
		StretchBlt(hdc,0,600,1024,160,hdcBanner,0,0,784,160,SRCCOPY);
		SelectObject(hdcBanner,hbmOldHorizontal);
		DeleteDC(hdcBanner);
	} else if (mode == 2) {// battle mode
		HDC hdcBanner = CreateCompatibleDC(hdc);
		HBITMAP hbmOldHorizontal = (HBITMAP)SelectObject(hdcBanner,hbitmap[27]); //hbitmap[27]=".\\HUD\\CharBar1024.bmp" no inventory button lower screen background
		StretchBlt(hdc,0,600,1024,160,hdcBanner,0,0,1021,160,SRCCOPY);
		SelectObject(hdcBanner,hbmOldHorizontal);
		DeleteDC(hdcBanner);
	}
	EndPaint (hwnd, &ps);
	return 0;
}
LRESULT Core::OnMainLBUTTONDOWN(HWND hwnd) {
	if(mode==4) {//dialogue mode click the lower screen to continue to the next page dialogue picture
		pNextPage->Play();
		InvalidateRect(hwnd, nullptr,true);
		mouseclick=true;
	}
	return 0;
}
LRESULT Core::OnMainCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam))	{
		case ITEMLISTBOX: {
			switch(HIWORD(wParam)) {
				case CBN_SELCHANGE:	{
					int lbItem = (int)SendMessage(hwndItemListBox,CB_GETCURSEL,0,0);
					itemindex=static_cast<int>(SendMessage(hwndItemListBox,CB_GETITEMDATA,lbItem,0));
					return true;
					}
			}
			return true;
		}
		case SKILLDROPBOX: {
			switch(HIWORD(wParam)) {
				case CBN_SELCHANGE: {
					int lbItem = (int)SendMessage(hwndSkillDropBox,CB_GETCURSEL,0,0);
					skillindex=static_cast<int>(SendMessage(hwndSkillDropBox,CB_GETITEMDATA,lbItem,0));
					return true;
				}
			}
			return true;
		}
		case ATTACKBUTTON: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					userchoice = 1;
					break;
					}
			}
			return true;
		}
		case MOVEBUTTON: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					userchoice = 2;
					break;
				}
			}
			return true;
		}
		case USEITEMBUTTON:	{
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					userchoice = 3;
					break;
				}
			}
			return true;
		}
		case USESKILLBUTTON: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					userchoice = 4;
					break;
				}
			}
			return true;
		}
		case JOURNALBUTTON: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					if (mode != 0) {//disable the button for intro animation
						if (jornalclicked == false) {//not clicked->jornal is not opened
							pButtonOpen->Play();
							jornalclicked = true;
							mapclicked = false;
							statusclicked = false;
							mode = 3;
							whichbutton = "journal";
						} else {
							pButtonClose->Play();
							jornalclicked = false;
							whichbutton = "";
							mode = 1;
						}
					}
					break;
				}
			}
			return true;
		}
		case MAPBUTTON:	{
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					if (mode != 0) {//disable the button for intro animation
						if (mapclicked == false) {//not clicked->map is not opened
							pButtonOpen->Play();
							mapclicked = true;
							jornalclicked = false;
							statusclicked = false;
							whichbutton = "map";
							mode = 3;
						} else {//if map is already open, turn off the map
							pButtonClose->Play();
							mapclicked = false;
							whichbutton = "";
							mode = 1;
						}
					}
					break;
				}
			}
			return true;
		}
		case STATUSBUTTON: {
			switch(HIWORD(wParam)) {
				case BN_CLICKED: {
					if (mode != 0) {//disable the button for intro animation
						if (statusclicked == false) {//not clicked->status is not opened
							pButtonOpen->Play();
							statusclicked = true;
							mapclicked = false;
							jornalclicked = false;
							whichbutton = "stat";
							mode = 3;
						} else {
							pButtonClose->Play();
							statusclicked = false;
							whichbutton = "";
							mode = 1;
						}
					}
					break;
				}
			}
			return true;
		}
	}
	return true;
}
LRESULT Core::OnChildMouseMove(HWND hChild, LPARAM lParam) {
	for (int i = 0; i < 4; ++i) {
		if (singlestate == 1 || singlestate == 3) {
			break;
		}
		if (singlestate == 4 && i > 0) {
			break;
		}
		if (mode == 1 && static_cast<Player*>(sprites[i])->getWalkEngine()->hold && static_cast<Player*>(sprites[i])->getWalkEngine()->movingqueue.empty()) {
			if (GET_X_LPARAM(lParam) < sprites[i]->getCamera()->getRight() - sprites[i]->getCamera()->getLeft() && GET_Y_LPARAM(lParam) < sprites[i]->getCamera()->getBottom() - sprites[i]->getCamera()->getTop()) {// if mouse click X < 1024 && Y < 600->mouse could click in the bottom HUD screen which we do not receive it as moving command
				static_cast<Player*>(sprites[i])->getWalkEngine()->cursorX = GET_X_LPARAM(lParam);
				static_cast<Player*>(sprites[i])->getWalkEngine()->cursorY = GET_Y_LPARAM(lParam);
				// if (hold && clickqueue.empty()) { //if we hold mouse left key, then we help system send WM_LBUTTONDOWN message to notify the new updating cursor position.
				SendMessage(hChild, WM_LBUTTONDOWN, 0, MAKELPARAM(static_cast<Player*>(sprites[i])->getWalkEngine()->cursorX, static_cast<Player*>(sprites[i])->getWalkEngine()->cursorY));//cursorXY never change from last mouse button down, send back to recevie new global large map MouseXY
																					 //  }//check if clickqueue has pre determined path to go, if there are some, then don't send WMLBUTTONDOWN message, or it will destroy all pre record the mouse clicks.
																					 //eve you click once, before WM_LBUTTONUP set hold false, there still will be a lot of message of WM_MOUSEMOVE with hold=true come in then caused WM_LBUTTONDOWN message sent destroy the clickqueue
			}
		}
	}
	return 0;
}
LRESULT Core::OnChildLBUTTONDOWN(HWND hChild, WPARAM wParam, LPARAM lParam) {
	SetFocus(hChild);
	//pExplore->hold = true;
	switch(mode) {
	case 0:	{
		if(ycounter>1400) {//intro animation ends, and the waiting mouse left button click has been done on upper screen, switch to the explore mode
			delete pIntro;
			pMapGraph->getMap("main_map")->playBGM();
			mode=4;
			ShowWindow(hwndJornalBT, SW_HIDE);
			ShowWindow(hwndMapBT, SW_HIDE);
			ShowWindow(hwndStatusBT, SW_HIDE);
			ShowWindow(hwndItemListBox, SW_HIDE);
			InvalidateRect(hwnd, nullptr, true);//clear lower screen picture and buttons
		} else {//skip the intro animation to the game guide help
			ycounter = 1401;
		}
	}
	break;
	case 1:	{
		for (int i = 0; i < 4; ++i) {
			if (singlestate == 1 || singlestate == 3) {
				break;
			}
			if (singlestate == 4 && i > 0) {
				break;
			}
			if (wParam & MK_SHIFT) {
				int localX = GET_X_LPARAM(lParam);
				int localY = GET_Y_LPARAM(lParam);
				if (localX < sprites[i]->getCamera()->getRight() - sprites[i]->getCamera()->getLeft() && localY < sprites[i]->getCamera()->getBottom() - sprites[i]->getCamera()->getTop()) {// if mouse click X < 1024 && Y < 600->mouse could click in the bottom HUD screen which we do not receive it as moving command
					int globalX = sprites[0]->getCamera()->getLeft() + localX;//use main character's camera left top as the same order
					int globalY = sprites[0]->getCamera()->getTop() + localY;//use main character's camera left top as the same order
					//continue;//if the player is not in the same map: someone enter the new map faster than the player
					mouseripple->oneTimeDrawStart(globalX, globalY, sprites[0]->getCurrentMap()->getTileSize(), sprites[0]->getCurrentMap()->getTileSize(), sprites[0]->getCamera());
					//static_cast<Player*>(sprites[i])->moveTo(globalX, globalY, pMapGraph, sprites[0]->getCurrentMap());
					sprites[i]->getWalkEngine()->getMovingQueue().push({make_pair(globalX, globalY), sprites[i]->getCurrentMap()->getMapID()});
					//pExplore->movingqueue.push(make_pair(globalX, globalY));
					//return 0;
					continue;
				}
			}
			static_cast<Player*>(sprites[i])->getWalkEngine()->hold = true;//don't put hold = true in the shift + click part, or if you shift click -> turn on the hold, the following part will sendmessage of their previous localXY and mess up the shift click function.
			if (GET_X_LPARAM(lParam) < sprites[i]->getCamera()->getRight() - sprites[i]->getCamera()->getLeft() && GET_Y_LPARAM(lParam) < sprites[i]->getCamera()->getBottom() - sprites[i]->getCamera()->getTop()) {
				// if mouse click X < 1024 && Y < 600->mouse could click in the bottom HUD screen which we do not receive it as moving command
				//if in the FinalPixelDelivery mode, new changed MouseXY could directly access to linear moving skipped Astar without considering obstacles,
				//so as long as in this mode, the final destination MouseXY cannot changed or it will let the character moving linearly pass through walls
				queue<TARGET> empty;
				swap(static_cast<Player*>(sprites[i])->getWalkEngine()->movingqueue, empty); //clear the click queue
				//swap(pExplore->movingqueue, empty); //clear the click queue

				//cursorX = LOWORD(lParam);
				//cursorY = HIWORD(lParam);
				/*
				Do not use the LOWORD or HIWORD macros to extract the x - and y - coordinates of the cursor position
				because these macros return incorrect results on systems with multiple monitors.
				Systems with multiple monitors can have negative x - and y - coordinates, and LOWORD and HIWORD treat the coordinates as unsigned quantities.
				*/
				int tempX = GET_X_LPARAM(lParam);
				int tempY = GET_Y_LPARAM(lParam);
				
				static_cast<Player*>(sprites[i])->getWalkEngine()->cursorX = GET_X_LPARAM(lParam);
				static_cast<Player*>(sprites[i])->getWalkEngine()->cursorY = GET_Y_LPARAM(lParam);
				//use main character's camera left top as the same order
				int globalX = sprites[0]->getCamera()->getLeft() + GET_X_LPARAM(lParam);//use main character's camera left top as the same order
				int globalY = sprites[0]->getCamera()->getTop() + GET_Y_LPARAM(lParam);
				mouseripple->oneTimeDrawStart(globalX, globalY, sprites[0]->getCurrentMap()->getTileSize(), sprites[0]->getCurrentMap()->getTileSize(), sprites[0]->getCamera());
				//continue;//if the player is not in the same map: someone enter the new map faster than the player, use the moveto function to let the teammate first go the player's main then to the player's place
				static_cast<Player*>(sprites[i])->moveTo(globalX, globalY, pMapGraph, sprites[0]->getCurrentMap());
			}
		}
	}
	break;
	case 2:	{//battle mode
		if (GET_X_LPARAM(lParam) < sprites[0]->getCamera()->getRight() - sprites[0]->getCamera()->getLeft() && GET_Y_LPARAM(lParam) < sprites[0]->getCamera()->getBottom() - sprites[0]->getCamera()->getTop()) {
			//pBattle->getMouseXY(GET_X_LPARAM(lParam), pExplore->cursorY = GET_Y_LPARAM(lParam));
		}
	}
	break;
	case 4: {//click upper screen, the dialogue also continue to the next page.
		pNextPage->Play();
		InvalidateRect(hwnd,nullptr,true);
		mouseclick=true;
	}
	break;
	default:
		break;
	}
	return 0;
}
LRESULT Core::OnChildPaint(HWND hChild, HDC childDc) {
	PAINTSTRUCT ps;
	if(ycounter<=1400)//intro anitmation not finished: the large picture is 800x2000, each frame shows 800x600 start from (0,0), (0,3), (0,6)... each time shows 800 width and 600 height
	{
		if (static_cast<Player*>(sprites[0])->getWalkEngine()->animationTimer.ElapsedTime() < 1/60.0) { //1s moving 60 pixels then, each pixel uses 1 / 60.0 second
			return 0;
		} else {
			static_cast<Player*>(sprites[0])->getWalkEngine()->animationTimer.Reset();
		}
		pGraphic->DrawScreen(hChild,hbitmap[23],0,0,1024,600,0,ycounter,800,600); //hbitmap[23]=".\\Dialogue\\background.bmp"--> it is the intro anitmation about story background
		++ycounter;														//ycounter is the starting coordinates of Y-axis of the large intro background picture:hbitmap[23]
	}
	else//ycounter > 1400 finished intro animation, automatically shows the game guid help
	{

		if(mode==0)//if this is just finished intro animation, show the game guide help
		{
			BeginPaint (hChild, &ps) ;
			HDC hdcBanner = CreateCompatibleDC(childDc);
			HBITMAP hbmOldHorizontal = (HBITMAP)SelectObject(hdcBanner,hbitmap[24]); //hbitmap[24]=".\\Dialogue\\help.bmp"-->game guide
			StretchBlt(childDc,0,0,1024,600,hdcBanner,0,0,1024,600,SRCCOPY);
			SelectObject(hdcBanner,hbmOldHorizontal);
			DeleteObject(hbmOldHorizontal);
			DeleteDC(hdcBanner);
			EndPaint (hChild, &ps);				
		}
	}
	InvalidateRect(hChild,nullptr,false);
	return 0;
}

LRESULT Core::OnKeyDown(WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
	case VK_F1:
		if (f1pressed) {
			f1pressed = false;
		} else {
			f1pressed = true;
		}
		break;
	case VK_F2:
		if (f2pressed) {
			f2pressed = false;
		} else {
			f2pressed = true;
		}
		break;
	case VK_F3:
		if (f3pressed) {
			f3pressed = false;
		} else {
			f3pressed = true;
		}
		break;
	case VK_F4:
		if (mode == 1) {
			if (f4pressed) {
				f4pressed = false;
			} else {
				f4pressed = true;
			}
		}
		break;
	case VK_F5:
		if (f5pressed) {
			f5pressed = false;
		} else {
			f5pressed = true;
		}
		break;
	case VK_F6:
		if (f6pressed) {
			f6pressed = false;
		} else {
			f6pressed = true;
		}
		break;
	case VK_F7:
		if (f7pressed) {
			f7pressed = false;
		} else {
			f7pressed = true;
		}
		break;
	default:
		break;
	}
	return 0;
}

LRESULT Core::OnMouseWheel(WPARAM wParam, LPARAM lParam) {
	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (f3pressed) {
			if (zDelta < 0) {
				++I3DL2;
			} else {
				--I3DL2;
			}
			pSoundDevice->GetXaudio2Ptr()->SetEffectParameters(I3DL2);
			return 0;
	}
	BGMVolDispTimer.Reset();
	BGMVolDispSwitch = true;
	if (zDelta < 0) {
		globalBGMVolume -= 0.01f;
		sprites[0]->getCurrentMap()->getBGM()->setVolume(max(0.0f, globalBGMVolume));
	} else {
		globalBGMVolume += 0.01f;
		sprites[0]->getCurrentMap()->getBGM()->setVolume(min(1.0f, globalBGMVolume));
	}
	return 0;
}
