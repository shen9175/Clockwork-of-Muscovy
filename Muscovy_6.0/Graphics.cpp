#include <Windows.h>
#include <WinGDI.h>
#include <windowsx.h>
#include <Wincodec.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <queue>
#include <ctime>
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
#include <fstream>
#include <iostream>
#include <string>
using namespace std;


// Creates a stream object initialized with the data from an executable resource.
IStream * Graphic::CreateStreamOnResource(int resourceID, LPCSTR lpType)
{
	// initialize return value
	IStream * ipStream = NULL;

	// find the resource
	HRSRC hrsrc = FindResource(NULL,MAKEINTRESOURCE(resourceID), lpType);
	if (hrsrc == NULL)
		goto Return;

	// load the resource
	DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
	HGLOBAL hglbImage = LoadResource(NULL, hrsrc);
	if (hglbImage == NULL)
		goto Return;

	// lock the resource, getting a pointer to its data
	LPVOID pvSourceResourceData = LockResource(hglbImage);
	if (pvSourceResourceData == NULL)
		goto Return;

	// allocate memory to hold the resource data
	HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
	if (hgblResourceData == NULL)
		goto Return;

	// get a pointer to the allocated memory
	LPVOID pvResourceData = GlobalLock(hgblResourceData);
	if (pvResourceData == NULL)
		goto FreeData;

	// copy the data from the resource to the new memory block
	CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
	GlobalUnlock(hgblResourceData);

	// create a stream on the HGLOBAL containing the data
	if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
		goto Return;

FreeData:
	// couldn't create stream; free the memory
	GlobalFree(hgblResourceData);

Return:
	// no need to unlock or free the resource
	return ipStream;
}
IWICBitmapSource * Graphic::LoadBitmapFromStream(IStream * ipImageStream)
{
	// initialize return value
	IWICBitmapSource * ipBitmap = NULL;

	// load WIC's PNG decoder
	IWICBitmapDecoder * ipDecoder = NULL;
	CoInitializeEx(nullptr, 0);//don't know why encapsulate it in class, it need coinitialize
	HRESULT hr = CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder));
	CoUninitialize();
	if (FAILED(hr))
		goto Return;
	
	// load the PNG
	if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
		goto ReleaseDecoder;

	// check for the presence of the first frame in the bitmap
	UINT nFrameCount = 0;
	if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
		goto ReleaseDecoder;

	// load the first frame (i.e., the image)
	IWICBitmapFrameDecode * ipFrame = NULL;
	if (FAILED(ipDecoder->GetFrame(0, &ipFrame)))
		goto ReleaseDecoder;

	// convert the image to 32bpp BGRA format with pre-multiplied alpha
	//   (it may not be stored in that format natively in the PNG resource,
	//   but we need this format to create the DIB to use on-screen)
	WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
	ipFrame->Release();

ReleaseDecoder:
	ipDecoder->Release();
Return:
	return ipBitmap;
}
// Creates a 32-bit DIB from the specified WIC bitmap.
HBITMAP Graphic::CreateHBITMAP(IWICBitmapSource * ipBitmap)
{
	// initialize return value
	HBITMAP hbmp = NULL;

	// get image attributes and check for valid image
	UINT width = 0;
	UINT height = 0;
	if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
		goto Return;

	// prepare structure giving bitmap information (negative height indicates a top-down DIB)
	BITMAPINFO bminfo;
	ZeroMemory(&bminfo, sizeof(bminfo));
	bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bminfo.bmiHeader.biWidth = width;
	bminfo.bmiHeader.biHeight = -((LONG) height);
	bminfo.bmiHeader.biPlanes = 1;
	bminfo.bmiHeader.biBitCount = 32;
	bminfo.bmiHeader.biCompression = BI_RGB;

	// create a DIB section that can hold the image
	void * pvImageBits = NULL;
	HDC hdcScreen = GetDC(NULL);
	hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
	ReleaseDC(NULL, hdcScreen);
	if (hbmp == NULL)
		goto Return;

	// extract the image into the HBITMAP
	const UINT cbStride = width * 4;
	const UINT cbImage = cbStride * height;
	if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE *>(pvImageBits))))
	{
		// couldn't extract image; delete HBITMAP
		DeleteObject(hbmp);
		hbmp = NULL;
	}

Return:
	return hbmp;
}
HBITMAP Graphic::LoadSplashImage(int resourceID)
{
	HBITMAP hbmpSplash = NULL;

	// load the PNG image data into a stream
	IStream * ipImageStream = CreateStreamOnResource(resourceID, "PNG");
	if (ipImageStream == NULL)
		goto Return;

	// load the bitmap with WIC
	IWICBitmapSource * ipBitmap = LoadBitmapFromStream(ipImageStream);
	if (ipBitmap == NULL)
		goto ReleaseStream;

	// create a HBITMAP containing the image
	hbmpSplash = CreateHBITMAP(ipBitmap);
	ipBitmap->Release();

ReleaseStream:
	ipImageStream->Release();
Return:
	return hbmpSplash;
}

//draw a specific pic from (picX,picY) with picW, picH to the screen from (screenX, screenY) with screeW and screenH with double buffer method: strech if two sizes are different
void Graphic::DrawScreen(HWND hWnd,  HBITMAP pic,int screenX, int screenY, int screenW, int screenH,int picX, int picY,int picW, int picH)
{
	HDC hdcfront=GetDC(hWnd);
	HDC hdcMem=CreateCompatibleDC(hdcfront);
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,ResX,ResY);//build 1024x600 screen bitmap
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen);
	
	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,pic);
	StretchBlt(hdcMem,screenX,screenY,screenW,screenH,hdcBitmapBackground,picX,picY,picW,picH,SRCCOPY); //copy new pic to memory buffer (hdcMem)
	SelectObject(hdcBitmapBackground,oldbackground);
	DeleteDC(hdcBitmapBackground);

	BitBlt(hdcfront,screenX,screenY,screenW,screenH,hdcMem,screenX,screenY,SRCCOPY);//copy mem buffer to front screen
	DeleteObject(screen);
	SelectObject(hdcMem,oldscreen);
	DeleteDC(hdcMem);

	ReleaseDC(hWnd,hdcfront);
}



void Graphic::DrawScreen(HWND hWnd, Character* p)
{
	HBITMAP background = nullptr;
	switch(p->getCurrentMap()->getMapID())
	{
	case 0:
		background=pCore->hbitmap[0];
		break;
	case 1:
		background=pCore->hbitmap[10];
		break;
	case 2:
		background=pCore->hbitmap[11];
		break;
	case 3:
		background=pCore->hbitmap[12];
		break;
	case 4:
		background=pCore->hbitmap[9];
		break;
	}

	HDC hdcfront=GetDC(hWnd);
	HDC hdcMem=CreateCompatibleDC(hdcfront);
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,ResX,ResY);  //create a new bitmap for a new frame

	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen/*background*/);//bind the new bitmap to memory dc and save the old bitmap previous binded with hdcMem
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,background);//bind background bitmap with background dc and save old bitmap prevoius binded with background dc

	StretchBlt(hdcMem,0,0, p->getCamera()->getRight() - p->getCamera()->getLeft(),p->getCamera()->getBottom() - p->getCamera()->getTop(), hdcBitmapBackground,p->getCamera()->getLeft(), p->getCamera()->getTop(), p->getCamera()->getRight() - p->getCamera()->getLeft(),p->getCamera()->getBottom() - p->getCamera()->getTop(),SRCCOPY);//map smaller than screen?
	//release selected background bmp by hdc, or it will not display background in the second screen. bmp only can be selected by one hdc
	SelectObject(hdcBitmapBackground,oldbackground);//resume previous bined bitmap back to background dc
	DeleteDC(hdcBitmapBackground);//delete background dc

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255;//transparency value between 0-255
	bf.AlphaFormat = AC_SRC_ALPHA;

	//pCore->teampop->TeamPoping(pCore->sprites, pCore->f6pressed);

	vector<Character*> zbuffer(pCore->sprites.cbegin(), pCore->sprites.cend());//simulate zbuffer to sort the front and back according to y value in 2D
	sort(zbuffer.begin(), zbuffer.end(), [](Character* a, Character* b) {return a->getPosition()->getY() < b->getPosition()->getY();});
	for(int i = 0;i < static_cast<int>(zbuffer.size());i++)
	{
		if(zbuffer[i]->getCurrentMap()->getMapID() == p->getCurrentMap()->getMapID()) //if it is in the current map
		{
			if(zbuffer[i]->isAlive()==true)//if it is still alive
			{
				//if it is in the current screen
				if(zbuffer[i]->getPosition()->getX() >= p->getCamera()->getLeft() && zbuffer[i]->getPosition()->getX() < p->getCamera()->getRight() && zbuffer[i]->getPosition()->getY() >= p->getCamera()->getTop() && zbuffer[i]->getPosition()->getY() < p->getCamera()->getBottom())
				{
					HDC hdcBitmapSprite=CreateCompatibleDC(hdcfront);
					HBITMAP oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,zbuffer[i]->getCurrentSprite());
					BITMAP bitmaptemp;
					GetObject(zbuffer[i]->getCurrentSprite(),sizeof(bitmaptemp),&bitmaptemp);
					int x = zbuffer[i]->getPosition()->getX() - p->getCamera()->getLeft() - p->getCurrentMap()->getTileSize() / 2;
					int y = zbuffer[i]->getPosition()->getY() - p->getCamera()->getTop() - p->getCurrentMap()->getTileSize();
					if (zbuffer[i]->getName() == "Engineer") {//player will not half body out of screen since camera is following the player.
						AlphaBlend(hdcMem,(x < 0 ? 0 : x),(y < 0 ? 0 : y),p->getCurrentMap()->getTileSize(),p->getCurrentMap()->getTileSize(),hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
					} else {// other NPC half body can be out of screen, since camera is not following them.
						AlphaBlend(hdcMem,x ,y ,p->getCurrentMap()->getTileSize(),p->getCurrentMap()->getTileSize(),hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
					}
					SelectObject(hdcBitmapSprite,oldsprites);
					DeleteDC(hdcBitmapSprite);
				}
			}
		}
	}
	pCore->mouseripple->oneTimeDraw(hdcMem);
	if (pCore->f4pressed) {//draw collision layer for debug
		HDC hdcSquare = CreateCompatibleDC(hdcfront);
		int istart = p->getCamera()->getTop() / p->getCurrentMap()->getTileSize() * p->getCurrentMap()->getTileSize();
		int jstart = p->getCamera()->getLeft() / p->getCurrentMap()->getTileSize() * p->getCurrentMap()->getTileSize();
		int iend = p->getCamera()->getBottom() / p->getCurrentMap()->getTileSize() * p->getCurrentMap()->getTileSize();
		int jend = p->getCamera()->getRight() / p->getCurrentMap()->getTileSize() * p->getCurrentMap()->getTileSize();
		for (int i = istart; i <= iend; i += p->getCurrentMap()->getTileSize()) {
			for (int j = jstart; j <= jend; j += p->getCurrentMap()->getTileSize()) {
				if (i / p->getCurrentMap()->getTileSize() >= p->getCurrentMap()->getTileHeight() || j / p->getCurrentMap()->getTileSize() >= p->getCurrentMap()->getTileWidth()) {
					continue; //if not reach the map edge, display more row/col to fill the whole screen, else do not display more
				}
				if (p->getCurrentMap()->getMap()[i / p->getCurrentMap()->getTileSize()][j / p->getCurrentMap()->getTileSize()] == 'w') {
					HBITMAP oldsprites = (HBITMAP)SelectObject(hdcSquare, pCore->hbitmap[269]);
					BITMAP bitmaptemp;
					GetObject(pCore->hbitmap[269], sizeof(bitmaptemp), &bitmaptemp);
					AlphaBlend(hdcMem, j - p->getCamera()->getLeft(), i - p->getCamera()->getTop(), p->getCurrentMap()->getTileSize(), p->getCurrentMap()->getTileSize(), hdcSquare, 0, 0, bitmaptemp.bmWidth, bitmaptemp.bmHeight, bf);
					SelectObject(hdcSquare, oldsprites);
				}
			}
		}
		DeleteDC(hdcSquare);
	}
	int l, t, w, h;
	secondScreen.Poping(l, t, w, h, pCore->f5pressed);
	if (w != 0 || h != 0) {
		DrawScreen(hdcMem, pCore->sprites[pCore->currentMonitoredNPC], l, t, w, h); // 1/5 of the original screen
	}
	int ml, mt, mw, mh;
	smallMap.MapPoping(ml, mt, mw, mh, pCore->f2pressed, p->getCurrentMap());
	if (mw != 0 || mh != 0) {
		DrawRealTimeMap(hdcMem, p, ml, mt, mw, mh);
	}
	//DrawScreen(hdcMem, pCore->sprites[4], 808,446, 205,154);

				if (fpsTimer.ElapsedTime() >= 1.0f) {
					fpsTimer.Reset();
				}
				fpsTimer.Tick();
				int oldBKmode = SetBkMode(hdcMem, TRANSPARENT);
				if (pCore->f1pressed) {
					RECT textRect;
					char buffer[256];
					SetRect(&textRect, 0, 0, 300, 20);
					sprintf_s(buffer, "%lld %s", fpsTimer.TotalTicks(), "FPS");
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 20, 400, 40);
					sprintf_s(buffer, "%.2f %s", 1000.0f / fpsTimer.TotalTicks(), "mspf");
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 40, 400, 60);
					sprintf_s(buffer, "%s: %.2f s", "Total Game Time", pCore->gameTimer.ElapsedTime());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 60, 400, 80);
					sprintf_s(buffer, "%s: %s, MapID = %d", "current map", p->getCurrentMap()->getMapName().c_str(), p->getCurrentMap()->getMapID());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 80, 400, 100);
					sprintf_s(buffer, "%s: %d, %d", "Engineer position", p->getPosition()->getX(), p->getPosition()->getY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 100, 400, 120);
					sprintf_s(buffer, "%s: %d, %d", "Guardsman position", pCore->sprites[1]->getPosition()->getX(), pCore->sprites[1]->getPosition()->getY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 120, 400, 140);
					sprintf_s(buffer, "%s: %d, %d", "Chemist position", pCore->sprites[2]->getPosition()->getX(), pCore->sprites[2]->getPosition()->getY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 140, 400, 160);
					sprintf_s(buffer, "%s: %d, %d", "Philosopher position", pCore->sprites[3]->getPosition()->getX(), pCore->sprites[3]->getPosition()->getY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 160, 400, 180);
					sprintf_s(buffer, "%s: %d, %d", "Tour guide position", pCore->sprites[4]->getPosition()->getX(), pCore->sprites[4]->getPosition()->getY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 180, 400, 200);
					sprintf_s(buffer, "%s: %s, MapID = %d", "tour guide map", pCore->sprites[4]->getCurrentMap()->getMapName().c_str(), pCore->sprites[4]->getCurrentMap()->getMapID());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);


					SetRect(&textRect, 0, 200, 400, 220);
					sprintf_s(buffer, "%s: %d, %d", "Engineer Target", pCore->sprites[0]->getWalkEngine()->getTargetX(), pCore->sprites[0]->getWalkEngine()->getTargetY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 220, 400, 240);
					sprintf_s(buffer, "%s: %d, %d", "Guardsman Target", pCore->sprites[1]->getWalkEngine()->getTargetX(), pCore->sprites[1]->getWalkEngine()->getTargetY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 240, 400, 260);
					sprintf_s(buffer, "%s: %d, %d", "Chemist Target", pCore->sprites[2]->getWalkEngine()->getTargetX(), pCore->sprites[2]->getWalkEngine()->getTargetY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);

					SetRect(&textRect, 0, 260, 400, 280);
					sprintf_s(buffer, "%s: %d, %d", "Philosopher Target", pCore->sprites[3]->getWalkEngine()->getTargetX(), pCore->sprites[3]->getWalkEngine()->getTargetY());
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);


				}
				if (pCore->f3pressed) {
					RECT textRect;
					char buffer[256];
					SetTextColor(hdcMem, RGB(204, 50, 50));
					SetRect(&textRect, 0, 580, 400, 600);
					sprintf_s(buffer, "%s: %s", "Footstep Reverb Effect", pCore->pSoundDevice->GetXaudio2Ptr()->GetI3DL2_Name((pCore->I3DL2 % 30 + 30) % 30));
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);
					SetTextColor(hdcMem, RGB(0, 0, 0));
				}
				if (pCore->BGMVolDispSwitch) {
					if (pCore->BGMVolDispTimer.ElapsedTime() > 1.0f) {
						pCore->BGMVolDispSwitch = false;
					}
					RECT textRect;
					char buffer[256];
					SetTextColor(hdcMem, RGB(204, 50, 50));
					SetRect(&textRect, 0, 580, 400, 600);
					sprintf_s(buffer, "%s: %d %%", "BackGround Music Volume", static_cast<int>(p->getCurrentMap()->getBGM()->getVolume() * 100.0f));
					DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);
					SetTextColor(hdcMem, RGB(0, 0, 0));
				}
				if(!pCore->noticequeue->empty()) {
					if (!pCore->noticequeue->showing()) {//queue is not empty, and it is not showing(last piece message has showed more than 1 second and disappear. So now show the next mesaage
						pCore->noticequeue->playSound();//play notice sound only at this first time not in else statement or it will call it every time about 200-300 times in 1 seconds, the sound heard to be delayed
						pCore->noticequeue->resetTimer();
						pCore->noticequeue->setShowing(true);
					}
					if (pCore->noticequeue->getElapsedTime() > 1.0f) {//after showing this message more than 1 second, delete it and set showing as false then next time the queue is not empty, it will goes in the first if statement.
						pCore->noticequeue->pop();
						pCore->noticequeue->setShowing(false);
					} else {
							LOGFONT logfont;
							logfont.lfHeight = 40;
							logfont.lfWidth	= 0;
							logfont.lfEscapement = 0;
							logfont.lfOrientation = 0;
							logfont.lfWeight = FW_BOLD;//700
							logfont.lfItalic = FALSE;
							logfont.lfUnderline = FALSE;
							logfont.lfStrikeOut = FALSE;
							logfont.lfCharSet = ANSI_CHARSET;
							logfont.lfOutPrecision = OUT_DEFAULT_PRECIS; //OUT_DEVICE_PRECIS;
							logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
							logfont.lfQuality = DEFAULT_QUALITY; //PROOF_QUALITY 
							logfont.lfPitchAndFamily = DEFAULT_PITCH + FF_ROMAN;
							strcpy_s(logfont.lfFaceName, "Edwardian Script ITC");

							HFONT hFont = CreateFontIndirect(&logfont);
							HGDIOBJ oldFont = SelectObject(hdcMem, hFont);
							SetTextColor(hdcMem, RGB(204, 50, 50));
							HPEN hPen = CreatePen(PS_SOLID,2,0);
							HGDIOBJ oldPen = SelectObject(hdcMem,hPen);
							SetTextAlign(hdcMem,TA_CENTER);
							TextOut(hdcMem,512,550,pCore->noticequeue->front().c_str(),static_cast<int>(pCore->noticequeue->front().size()));
							SetTextAlign(hdcMem,TA_LEFT);
							SelectObject(hdcMem, oldPen);
							DeleteObject(hPen);
							SelectObject(hdcMem, oldFont);
							DeleteObject(hFont);
							SetTextColor(hdcMem, RGB(0, 0, 0));
					}
				}
				SetBkMode(hdcMem, oldBKmode);
	BitBlt(hdcfront,0,0,ResX,ResY,hdcMem,0,0,SRCCOPY);
	SelectObject(hdcMem,oldscreen);//resume previous binded bitmap back to memory dc
	DeleteObject(screen);//delete just created screen bitmap for a new frame;
	DeleteDC(hdcMem);//delete memory dc

	ReleaseDC(hWnd,hdcfront);
}

void Graphic::DrawScreen(HDC hdcfront, Character* p, int left, int top, int width, int height) {//for second screen


	HBITMAP background = nullptr;
	switch(p->getCurrentMap()->getMapID())
	{
	case 0:
		background=pCore->hbitmap[0];
		break;
	case 1:
		background=pCore->hbitmap[10];
		break;
	case 2:
		background=pCore->hbitmap[11];
		break;
	case 3:
		background=pCore->hbitmap[12];
		break;
	case 4:
		background=pCore->hbitmap[9];
		break;
	}

	HDC hdcMem=CreateCompatibleDC(hdcfront);
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,ResX,ResY);
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen);

	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,background);
	StretchBlt(hdcMem,0,0, p->getCamera()->getRight() - p->getCamera()->getLeft(),p->getCamera()->getBottom() - p->getCamera()->getTop(), 
	hdcBitmapBackground,p->getCamera()->getLeft(), p->getCamera()->getTop(), p->getCamera()->getRight() - p->getCamera()->getLeft(),p->getCamera()->getBottom() - p->getCamera()->getTop(),SRCCOPY);//map smaller than screen?
	SelectObject(hdcBitmapBackground,oldbackground);
	DeleteDC(hdcBitmapBackground);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255; //transparency value between 0-255
	bf.AlphaFormat = AC_SRC_ALPHA;
	vector<Character*> zbuffer(pCore->sprites.cbegin(), pCore->sprites.cend());//simulate zbuffer to sort the front and back according to y value in 2D
	sort(zbuffer.begin(), zbuffer.end(), [](Character* a, Character* b) {return a->getPosition()->getY() < b->getPosition()->getY();});
	for(int i = 0;i < static_cast<int>(zbuffer.size());i++)
	{
		if(zbuffer[i]->getCurrentMap()->getMapID() == p->getCurrentMap()->getMapID()) //if it is in the current map
		{
			if(zbuffer[i]->isAlive()==true)//if it is still alive
			{
				//if it is in the current screen
				if(zbuffer[i]->getPosition()->getX() >= p->getCamera()->getLeft() && zbuffer[i]->getPosition()->getX() < p->getCamera()->getRight() && zbuffer[i]->getPosition()->getY() >= p->getCamera()->getTop() && zbuffer[i]->getPosition()->getY() < p->getCamera()->getBottom())
				{
					HDC hdcBitmapSprite=CreateCompatibleDC(hdcfront);
					HBITMAP oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,zbuffer[i]->getCurrentSprite());
					BITMAP bitmaptemp;
					GetObject(zbuffer[i]->getCurrentSprite(),sizeof(bitmaptemp),&bitmaptemp);
					int x = zbuffer[i]->getPosition()->getX() - p->getCamera()->getLeft() - p->getCurrentMap()->getTileSize() / 2;
					int y = zbuffer[i]->getPosition()->getY() - p->getCamera()->getTop() - p->getCurrentMap()->getTileSize();
					if (i == 0) {//player will not half body out of screen since camera is following the player.
						AlphaBlend(hdcMem,(x < 0 ? 0 : x),(y < 0 ? 0 : y),p->getCurrentMap()->getTileSize(),p->getCurrentMap()->getTileSize(),hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
					} else {// other NPC half body can be out of screen, since camera is not following them.
						AlphaBlend(hdcMem,x ,y ,p->getCurrentMap()->getTileSize(),p->getCurrentMap()->getTileSize(),hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
					}
					SelectObject(hdcBitmapSprite,oldsprites);
					DeleteDC(hdcBitmapSprite);
				}
			}
		}
	}
	int oldmode = SetStretchBltMode(hdcfront, HALFTONE);
	StretchBlt(hdcfront,left,top, width,height,hdcMem,0,0, ResX, ResY, SRCCOPY);
	SetStretchBltMode(hdcfront, oldmode);

	SelectObject(hdcMem,oldscreen);
	DeleteObject(screen);
	DeleteDC(hdcMem);
}
void Graphic::DrawRealTimeMap(HDC hdcfront, Character* p, int left, int top, int width, int height) {


	HBITMAP background = nullptr;
	switch(p->getCurrentMap()->getMapID())
	{
	case 0:
		background=pCore->hbitmap[345];
		break;
	case 1:
		background=pCore->hbitmap[10];
		break;
	case 2:
		background=pCore->hbitmap[11];
		break;
	case 3:
		background=pCore->hbitmap[12];
		break;
	case 4:
		background=pCore->hbitmap[9];
		break;
	}
	HDC hdcMem=CreateCompatibleDC(hdcfront);
	BITMAP bg;
	GetObject(background,sizeof(bg),&bg);
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,bg.bmWidth,bg.bmHeight);
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen);

	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,background);
	BitBlt(hdcMem,0,0,bg.bmWidth,bg.bmHeight, hdcBitmapBackground, 0, 0, SRCCOPY);//map smaller than screen?
	SelectObject(hdcBitmapBackground,oldbackground);
	DeleteDC(hdcBitmapBackground);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255; //transparency value between 0-255
	bf.AlphaFormat = AC_SRC_ALPHA;
	vector<Character*> zbuffer(pCore->sprites.cbegin(), pCore->sprites.cend());//simulate zbuffer to sort the front and back according to y value in 2D
	sort(zbuffer.begin(), zbuffer.end(), [](Character* a, Character* b) {return a->getPosition()->getY() < b->getPosition()->getY();});
	for(int i = 0;i < static_cast<int>(zbuffer.size());i++)
	{
		if(zbuffer[i]->getCurrentMap()->getMapID() == p->getCurrentMap()->getMapID()) //if it is in the current map
		{
			if(zbuffer[i]->isAlive()==true)//if it is still alive
			{

					HDC hdcBitmapSprite=CreateCompatibleDC(hdcfront);
					HBITMAP oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,zbuffer[i]->getCurrentSprite());
					BITMAP bitmaptemp;
					GetObject(zbuffer[i]->getCurrentSprite(),sizeof(bitmaptemp),&bitmaptemp);
					int x;
					int y;
					int length;
					if (p->getCurrentMap()->getMapID() == 0) {
						x = static_cast<int>((zbuffer[i]->getPosition()->getX() - p->getCurrentMap()->getTileSize() / 2) * 0.2);//20% orginal size of the map
						y = static_cast<int>((zbuffer[i]->getPosition()->getY() - p->getCurrentMap()->getTileSize()) * 0.2);
						length = static_cast<int>(p->getCurrentMap()->getTileSize() * 0.4);//sprites are 2 times larger than original 0.2 of the original size for more clearly look
					} else {
						x = zbuffer[i]->getPosition()->getX() - p->getCurrentMap()->getTileSize() / 2;
						y = zbuffer[i]->getPosition()->getY() - p->getCurrentMap()->getTileSize();
						length = static_cast<int>(p->getCurrentMap()->getTileSize());
					}
					if (i == 0) {//player will not half body out of screen since camera is following the player.
						AlphaBlend(hdcMem, (x < 0 ? 0 : x), (y < 0 ? 0 : y), length, length, hdcBitmapSprite, 0, 0, bitmaptemp.bmWidth, bitmaptemp.bmHeight, bf);
					} else {// other NPC half body can be out of screen, since camera is not following them.
						AlphaBlend(hdcMem, x, y, length, length, hdcBitmapSprite, 0, 0, bitmaptemp.bmWidth, bitmaptemp.bmHeight, bf);
					}
					SelectObject(hdcBitmapSprite,oldsprites);
					DeleteDC(hdcBitmapSprite);
			}
		}
	}

	if (pCore->f4pressed) {
		HDC hdcSquare = CreateCompatibleDC(hdcfront);
		for (int i = 0; i < p->getCurrentMap()->getTileHeight(); ++i) {
			for (int j = 0; j < p->getCurrentMap()->getTileWidth(); ++j) {
				if (p->getCurrentMap()->getMap()[i][j] == 'w') {
					HBITMAP oldsprites = (HBITMAP)SelectObject(hdcSquare, pCore->hbitmap[269]);
					BITMAP bitmaptemp;
					GetObject(pCore->hbitmap[269], sizeof(bitmaptemp), &bitmaptemp);
					int x;
					int y;
					int length;
					if (p->getCurrentMap()->getMapID() == 0) {
						x = static_cast<int>((j * p->getCurrentMap()->getTileSize() + 0) * 0.2);
						y = static_cast<int>((i * p->getCurrentMap()->getTileSize() + 0) * 0.2);
						length = static_cast<int>(p->getCurrentMap()->getTileSize() * 0.2);
					} else {
						x = j * p->getCurrentMap()->getTileSize() + 0;
						y = i * p->getCurrentMap()->getTileSize() + 0;
						length = p->getCurrentMap()->getTileSize();
					}
					AlphaBlend(hdcMem, x, y, length, length, hdcSquare, 0, 0, bitmaptemp.bmWidth, bitmaptemp.bmHeight, bf);
					SelectObject(hdcSquare, oldsprites);
				}
			}
		}
		DeleteDC(hdcSquare);
	}
	int oldmode = SetStretchBltMode(hdcfront, HALFTONE);
	StretchBlt(hdcfront,left,top, width,height,hdcMem,0,0, bg.bmWidth, bg.bmHeight, SRCCOPY);
	SetStretchBltMode(hdcfront, oldmode);

	SelectObject(hdcMem,oldscreen);
	DeleteObject(screen);
	DeleteDC(hdcMem);

}


void Graphic::DrawMapMode(HWND hWnd, Character* p, HBITMAP target, HBITMAP player,HBITMAP smallmap,int mapX, int mapY,  int targetX, int targetY,int playerX, int playerY)
{
	HBITMAP background=pCore->hbitmap[0];
	HDC hdcfront=GetDC(hWnd);
	HDC hdcMem=CreateCompatibleDC(hdcfront);
	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255; //transparency value between 0-255
	bf.AlphaFormat = AC_SRC_ALPHA;    
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,ResX,ResY);
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen/*background*/);
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,background);
	StretchBlt(hdcMem,0,0,p->getCamera()->getRight() - p->getCamera()->getLeft(),p->getCamera()->getBottom() - p->getCamera()->getTop(),hdcBitmapBackground,p->getCamera()->getLeft(), p->getCamera()->getTop(), p->getCamera()->getRight() - p->getCamera()->getLeft(),p->getCamera()->getBottom() - p->getCamera()->getTop(),SRCCOPY);
	SelectObject(hdcBitmapBackground,oldbackground);
	DeleteDC(hdcBitmapBackground);

	HDC hdcBitmapSprite=CreateCompatibleDC(hdcfront);
	HBITMAP oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,smallmap);
	BITMAP bitmaptemp;
	GetObject(smallmap,sizeof(bitmaptemp),&bitmaptemp);
	//AlphaBlend(hdcMem,mapX,mapY,bitmaptemp.bmWidth,bitmaptemp.bmHeight,hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
	StretchBlt(hdcMem,mapX,mapY,bitmaptemp.bmWidth,bitmaptemp.bmHeight,hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,SRCCOPY);
	SelectObject(hdcBitmapSprite,oldsprites);
	DeleteDC(hdcBitmapSprite);

	hdcBitmapSprite=CreateCompatibleDC(hdcfront);
	oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,target);
	bitmaptemp;
	GetObject(target,sizeof(bitmaptemp),&bitmaptemp);
	AlphaBlend(hdcMem,targetX,targetY,bitmaptemp.bmWidth,bitmaptemp.bmHeight,hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
	SelectObject(hdcBitmapSprite,oldsprites);
	DeleteDC(hdcBitmapSprite);

	hdcBitmapSprite=CreateCompatibleDC(hdcfront);
	oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,player);
	bitmaptemp;
	GetObject(player,sizeof(bitmaptemp),&bitmaptemp);
	AlphaBlend(hdcMem,playerX,playerY,bitmaptemp.bmWidth,bitmaptemp.bmHeight,hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
	SelectObject(hdcBitmapSprite,oldsprites);
	DeleteDC(hdcBitmapSprite);


	BitBlt(hdcfront,0,0,ResX,ResY,hdcMem,0,0,SRCCOPY);
	SelectObject(hdcMem,oldscreen);
	DeleteObject(screen);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd,hdcfront);

}




void Graphic::DrawScreen(HWND hWnd,  HBITMAP sprites, HBITMAP background,int spritesX, int spritesY,int backGroundX, int backGroundY)
{
	//PAINTSTRUCT ps;
	//HDC hdcfront=BeginPaint(hWnd,&ps)

	HDC hdcfront=GetDC(hWnd);
	HDC hdcMem=CreateCompatibleDC(hdcfront);
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,ResX,ResY);

	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen/*background*/);
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,background);
	StretchBlt(hdcMem,0,0,ResX,ResY,hdcBitmapBackground,backGroundX,backGroundY,ResX,ResY,SRCCOPY);
	SelectObject(hdcBitmapBackground,oldbackground);
	DeleteDC(hdcBitmapBackground);

	HDC hdcBitmapSprite=CreateCompatibleDC(hdcfront);
	HBITMAP oldsprites=(HBITMAP)SelectObject(hdcBitmapSprite,sprites);
	BITMAP bitmaptemp;
	GetObject(sprites,sizeof(bitmaptemp),&bitmaptemp);
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = 255; //transparency value between 0-255
	bf.AlphaFormat = AC_SRC_ALPHA;    
	AlphaBlend(hdcMem,spritesX,spritesY,bitmaptemp.bmWidth,bitmaptemp.bmHeight,hdcBitmapSprite,0,0,bitmaptemp.bmWidth,bitmaptemp.bmHeight,bf);
	SelectObject(hdcBitmapSprite,oldsprites);
	DeleteDC(hdcBitmapSprite);

	BitBlt(hdcfront,0,0,ResX,ResY,hdcMem,0,0,SRCCOPY);
	SelectObject(hdcMem,oldscreen);
	DeleteObject(screen);
	DeleteDC(hdcMem);


	ReleaseDC(hWnd,hdcfront);
}

void Graphic::ShowStat(HWND hWnd) {
	HBITMAP pic = pCore->hbitmap[25]; int screenX = 0; int screenY = 0; int screenW = 1024; int screenH = 600; int picX = 0; int picY = 0; int picW = 1023; int picH = 599;
	HDC hdcfront=GetDC(hWnd);
	HDC hdcMem=CreateCompatibleDC(hdcfront);
	HBITMAP screen=CreateCompatibleBitmap(hdcfront,ResX,ResY);//build 1024x600 screen bitmap
	HBITMAP oldscreen=(HBITMAP)SelectObject(hdcMem,screen);

	HDC hdcBitmapBackground=CreateCompatibleDC(hdcfront);
	HBITMAP oldbackground=(HBITMAP)SelectObject(hdcBitmapBackground,pic);
	StretchBlt(hdcMem,screenX,screenY,screenW,screenH,hdcBitmapBackground,picX,picY,picW,picH,SRCCOPY); //copy new pic to memory buffer (hdcMem)//put background first
	SelectObject(hdcBitmapBackground,oldbackground);
	DeleteDC(hdcBitmapBackground);

	//then put text to hdMem:
	LOGFONT logfont;
	logfont.lfHeight = 40;
	logfont.lfWidth	= 0;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfWeight = FW_BOLD;//700
	logfont.lfItalic = FALSE;
	logfont.lfUnderline = FALSE;
	logfont.lfStrikeOut = FALSE;
	logfont.lfCharSet = ANSI_CHARSET;
	logfont.lfOutPrecision = OUT_DEFAULT_PRECIS; //OUT_DEVICE_PRECIS;
	logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logfont.lfQuality = DEFAULT_QUALITY; //PROOF_QUALITY 
	logfont.lfPitchAndFamily = DEFAULT_PITCH + FF_ROMAN;
	strcpy_s(logfont.lfFaceName, "Edwardian Script ITC");

	HFONT hFont = CreateFontIndirect(&logfont);
	HGDIOBJ oldFont = SelectObject(hdcMem, hFont);//backup old font, need to be HGDIOBJ type not HFONT type
	HPEN hPen = CreatePen(PS_SOLID,2,0);
	HGDIOBJ oldPen = SelectObject(hdcMem,hPen);//backup old pen, need to be HGDIOBJ type not HPEN type
	RECT textRect;
	SetRect (&textRect, 176, 285, 366, 303);
	int oldBKmode = SetBkMode(hdcMem, TRANSPARENT);
	SetTextColor(hdcMem, RGB(204, 50, 50));
	char buffer[256];
	sprintf_s(buffer,"%d/%d",pCore->sprites[pCore->currentPlayer]->getCurrentHP(),pCore->sprites[pCore->currentPlayer]->getMajor()->getHealth());
	//DrawText(hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	TextOut( hdcMem,176,273, buffer,static_cast<int>(strlen(buffer)));
	SetRect (&textRect, 203, 320, 394, 336);
	sprintf_s(buffer,"%d",pCore->sprites[pCore->currentPlayer]->getLevel());
	TextOut( hdcMem,203,310, buffer, static_cast<int>(strlen(buffer)));
	//DrawText( hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	SetRect (&textRect, 278, 361, 398, 383);
	sprintf_s(buffer,"%d",((Player*)pCore->sprites[pCore->currentPlayer])->getExperience());
	//DrawText( hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	TextOut( hdcMem,278,349, buffer, static_cast<int>(strlen(buffer)));
	SetRect (&textRect, 248, 396, 420, 415);
	sprintf_s(buffer,"%d",pCore->sprites[pCore->currentPlayer]->getMajor()->getDexterity());
	//DrawText( hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	TextOut( hdcMem,248,384, buffer, static_cast<int>(strlen(buffer)));
	SetRect (&textRect, 268, 432, 420, 456);
	sprintf_s(buffer,"%d",pCore->sprites[pCore->currentPlayer]->getMajor()->getWillPower());
	//DrawText( hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	TextOut( hdcMem,268,420, buffer, static_cast<int>(strlen(buffer)));
	SetRect (&textRect, 273, 470, 420, 489);
	sprintf_s(buffer,"%d",pCore->sprites[pCore->currentPlayer]->getMajor()->getEndurance());
	//DrawText( hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	TextOut( hdcMem,273,458, buffer, static_cast<int>(strlen(buffer)));
	SetRect (&textRect, 243, 506, 420, 530);
	sprintf_s(buffer,"%d",pCore->sprites[pCore->currentPlayer]->getMajor()->getWeaponSkill());
	//DrawText( hdcMem, buffer, -1, &textRect,DT_LEFT| DT_VCENTER);
	TextOut( hdcMem,243,494, buffer, static_cast<int>(strlen(buffer)));
	SetBkMode(hdcMem, oldBKmode);
	SelectObject(hdcMem, oldPen);//resume back old pen what ever it is
	DeleteObject(hPen);//delete the pen you created it just now
	SelectObject(hdcMem, oldFont);//resume back old font what ever it is
	DeleteObject(hFont);//delete the font you created it just now
	SetTextColor(hdcMem, RGB(0,0,0));


	BitBlt(hdcfront,screenX,screenY,screenW,screenH,hdcMem,screenX,screenY,SRCCOPY);//copy mem buffer to front screen
	SelectObject(hdcMem,oldscreen);
	DeleteObject(screen);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd,hdcfront);
}

ScreenPop::ScreenPop() { 
	left = static_cast<int>(4.0f / 5.0f * ResX);
	top = static_cast<int>(4.0f / 5.0 * ResY);
	width = static_cast<int>(1.0f / 5.0 * ResX);
	height = static_cast<int>(1.0f / 5.0f * ResY);
	state = 4;
	rollingspeed = 1;
	counter = 0;
	AniTimer.Reset(); 
}
ScreenPop::ScreenPop(int l, int t, int w, int h, float speed) : left(l), top(t), width(w), height(h), rollingspeed(speed) {
	state = 4;
	counter = 0;
	AniTimer.Reset();
}

//finite state machine for screen rolling out and in animation by F5 key toggling ON/OFF
void ScreenPop::MapPoping(int& l, int&  t, int& w, int& h, bool toggle, Map* map) {
	if (map->getMapID() == 0) {
		width = static_cast<int>(0.04 * map->getPixelWidth());
		height = static_cast<int>(0.04 * map->getPixelHeight());
	} else if (map->getMapID() == 4){
		width = static_cast<int>(0.15 * map->getPixelWidth());
		height = static_cast<int>(0.15 * map->getPixelHeight());
	} else {
		width = static_cast<int>(0.25 * map->getPixelWidth());
		height = static_cast<int>(0.25 * map->getPixelHeight());
	}
	switch (state) {
	case 1://opening
		if (AniTimer.ElapsedTime() > rollingspeed / 200.0f) {//hardcode as 200 step to finishing rolling
			++counter;
			l = static_cast<int>(ResX - counter / 100.0f * width);
			t = static_cast<int>(ResY - counter / 100.0f * height);
			w = static_cast<int>(counter / 100.0f * width);
			h = static_cast<int>(counter / 100.0f * height);
			AniTimer.Reset();
		}
		if (counter == 100) {
			state = 2;
		}
		if (toggle == false) {
			state = 3;//change to closing
			AniTimer.Reset();
		}
		break;
	case 2://has been open for some time
		l = ResX - width;
		t = ResY - height;
		w = width;
		h = height;
		if (toggle == false) {
			state = 3;
			AniTimer.Reset();
		}
		break;
	case 3://closing
		if (AniTimer.ElapsedTime() > rollingspeed / 200.0f) {//hardcode as 100 step to finishing rolling
			--counter;
			l = static_cast<int>(ResX - counter / 100.0f * width);
			t = static_cast<int>(ResY - counter / 100.0f * height);
			w = static_cast<int>(counter / 100.0f * width);
			h = static_cast<int>(counter / 100.0f * height);
			AniTimer.Reset();
		}
		if (counter == 0) {
			state = 4;
		}
		if (toggle == true) {//become opening
			state = 1;
			AniTimer.Reset();
		}
		break;
	case 4://closed
		l = ResX;
		t = ResY;
		w = 0;
		h = 0;
		if (toggle == true) {
			state = 1;
			AniTimer.Reset();
		}
		break;
	}
}

void ScreenPop::Poping(int& l, int&  t, int& w, int& h, bool toggle) {

	switch (state) {
	case 1://opening
		if (AniTimer.ElapsedTime() > rollingspeed / 200.0f) {//hardcode as 200 step to finishing rolling
			++counter;
			l = static_cast<int>(ResX - counter / 100.0f * width);
			t = static_cast<int>(ResY - counter / 100.0f * height);
			w = static_cast<int>(counter / 100.0f * width);
			h = static_cast<int>(counter / 100.0f * height);
			AniTimer.Reset();
		}
		if (counter == 100) {
			state = 2;
		}
		if (toggle == false) {
			state = 3;//change to closing
			AniTimer.Reset();
		}
		break;
	case 2://has been open for some time
		l = left;
		t = top;
		w = width;
		h = height;
		if (toggle == false) {
			state = 3;
			AniTimer.Reset();
		}
		break;
	case 3://closing
		if (AniTimer.ElapsedTime() > rollingspeed / 200.0f) {//hardcode as 100 step to finishing rolling
			--counter;
			l = static_cast<int>(ResX - counter / 100.0f * width);
			t = static_cast<int>(ResY - counter / 100.0f * height);
			w = static_cast<int>(counter / 100.0f * width);
			h = static_cast<int>(counter / 100.0f * height);
			AniTimer.Reset();
		}
		if (counter == 0) {
			state = 4;
		}
		if (toggle == true) {//become opening
			state = 1;
			AniTimer.Reset();
		}
		break;
	case 4://closed
		l = ResX;
		t = ResY;
		w = 0;
		h = 0;
		if (toggle == true) {
			state = 1;
			AniTimer.Reset();
		}
		break;
	}
}


void TeamPop::TeamPoping(vector<Character*> sprites, bool toggle) {
	//initial state is 2, 4 people mode, spread
	// 1. spreading, 2. spread, 3. gathering, 4. gathered
	switch (state) {
	case 1://spreading
		if (AniTimer.ElapsedTime() > 0.01) {//hardcode as 300 step to finishing rolling
			++counter;
			int row = sprites[0]->getPosition()->getRow();
			int col = sprites[0]->getPosition()->getCol();
			int x = sprites[0]->getPosition()->getX();
			int y = sprites[0]->getPosition()->getY();
			int tile = sprites[0]->getCurrentMap()->getTileSize();
			int width = sprites[0]->getCurrentMap()->getTileWidth();
			int height = sprites[0]->getCurrentMap()->getTileHeight();
			char** map = sprites[0]->getCurrentMap()->getMap();
			Map* Map = sprites[0]->getCurrentMap();
			if (firstime) {
				soundfx->Play();
				sprites[0]->getWalkEngine()->resetWalkEngine();
				sprites[1]->setCurrentMap(sprites[0]->getCurrentMap());
				sprites[2]->setCurrentMap(sprites[0]->getCurrentMap());
				sprites[3]->setCurrentMap(sprites[0]->getCurrentMap());
				sprites[1]->getPosition()->setDir(sprites[0]->getPosition()->getDir());
				sprites[2]->getPosition()->setDir(sprites[0]->getPosition()->getDir());
				sprites[3]->getPosition()->setDir(sprites[0]->getPosition()->getDir());
				sprites[1]->setAlive(true);
				sprites[2]->setAlive(true);
				sprites[3]->setAlive(true);
				if (row - 1 >= 0 && map[row - 1][col] != 'w') {
					GuardsmanDeltaX = 0;
					GuardsmanDeltaY =  -tile;
				} else {
					GuardsmanDeltaX = 0;
					GuardsmanDeltaY = 0;
					sprites[1]->getPosition()->moveCardinalDirectionPixel(x, y, Map);
				}
				if (row + 1 < height && col - 1 >= 0 && map[row + 1][col - 1] != 'w') {
					ChemistDeltaX = - tile;
					ChemistDeltaY = tile;
				} else {
					ChemistDeltaX = 0;
					ChemistDeltaY = 0;
					sprites[2]->getPosition()->moveCardinalDirectionPixel(x, y, Map);
				}
				if (col + 1 < width && map[row][col + 1] != 'w') {
					PhilosopherDeltaX = tile;
					PhilosopherDeltaY = 0;
				} else {
					PhilosopherDeltaX = 0;
					PhilosopherDeltaY = 0;
					sprites[3]->getPosition()->moveCardinalDirectionPixel(x, y, Map);
				}
				firstime = false;
			}
			if (GuardsmanDeltaX != 0 || GuardsmanDeltaY != 0) {
				sprites[1]->getPosition()->moveCardinalDirectionPixel(static_cast<int>(x + GuardsmanDeltaX / 10.0f * counter), static_cast<int>(y + GuardsmanDeltaY / 10.0f * counter), Map);
				if (counter == 10) {
					map[sprites[1]->getPosition()->getRow()][sprites[1]->getPosition()->getCol()] = 'w';
					sprites[1]->getWalkEngine()->setOccupiedTile(sprites[1]->getPosition()->getRow(), sprites[1]->getPosition()->getCol());
				}
			}
			if (ChemistDeltaX != 0 || ChemistDeltaY != 0) {
				sprites[2]->getPosition()->moveCardinalDirectionPixel(static_cast<int>(x + ChemistDeltaX / 10.0f * counter), static_cast<int>(y + ChemistDeltaY / 10.0f * counter), Map);
				if (counter == 10) {
					map[sprites[2]->getPosition()->getRow()][sprites[2]->getPosition()->getCol()] = 'w';
					sprites[2]->getWalkEngine()->setOccupiedTile(sprites[2]->getPosition()->getRow(), sprites[2]->getPosition()->getCol());
				}
			}
			if (PhilosopherDeltaX != 0 || PhilosopherDeltaY != 0) {
				sprites[3]->getPosition()->moveCardinalDirectionPixel(static_cast<int>(x + PhilosopherDeltaX / 10.0f * counter), static_cast<int>(y + PhilosopherDeltaY / 10.0f * counter), Map);
				if (counter == 10) {
					map[sprites[3]->getPosition()->getRow()][sprites[3]->getPosition()->getCol()] = 'w';
					sprites[3]->getWalkEngine()->setOccupiedTile(sprites[3]->getPosition()->getRow(), sprites[3]->getPosition()->getCol());
				}
			}
			if (counter == 10) {
				sprites[1]->getWalkEngine()->resetWalkEngine();//reset walkengine will set the TargetXY as itself position. so only after spreading, sprites[1-3] has itself position
				sprites[2]->getWalkEngine()->resetWalkEngine();
				sprites[3]->getWalkEngine()->resetWalkEngine();
				firstime = true;
				state = 2;
			}
			AniTimer.Reset();
		}
		/* disable gathering during spreading
		if (toggle == false) {
			state = 3;//change to closing
			AniTimer.Reset();
		}*/
		break;
	case 2://has been open for some time
		if (toggle == true) {
			state = 3;
			AniTimer.Reset();
		}
		break;
	case 3://closing
		if (AniTimer.ElapsedTime() > 0.01) {//hardcode as 100 step to finishing rolling
			--counter;
			int row = sprites[0]->getPosition()->getRow();
			int col = sprites[0]->getPosition()->getCol();
			int x = sprites[0]->getPosition()->getX();
			int y = sprites[0]->getPosition()->getY();
			int tile = sprites[0]->getCurrentMap()->getTileSize();
			int width = sprites[0]->getCurrentMap()->getTileWidth();
			int height = sprites[0]->getCurrentMap()->getTileHeight();
			char** map = sprites[0]->getCurrentMap()->getMap();
			Map* Map = sprites[0]->getCurrentMap();
			if (firstime) {
				soundfx->Play();
				sprites[0]->getWalkEngine()->resetWalkEngine();
				sprites[1]->getWalkEngine()->resetWalkEngine();
				sprites[2]->getWalkEngine()->resetWalkEngine();
				sprites[3]->getWalkEngine()->resetWalkEngine();
				if (sprites[1]->getCurrentMap() == Map) {
					GuardsmanDeltaX = sprites[1]->getPosition()->getX() - x;
					GuardsmanDeltaY = sprites[1]->getPosition()->getY() - y;
					if (GuardsmanDeltaX != 0 || GuardsmanDeltaY != 0) {
						sprites[1]->getCurrentMap()->getMap()[sprites[1]->getPosition()->getRow()][sprites[1]->getPosition()->getCol()] = '.';
						sprites[1]->getWalkEngine()->setOccupiedTile(0, 0);
					}
				} else {
					GuardsmanDeltaX = 0;
					GuardsmanDeltaY = 0;
					sprites[1]->getCurrentMap()->getMap()[sprites[1]->getPosition()->getRow()][sprites[1]->getPosition()->getCol()] = '.';
					sprites[1]->getWalkEngine()->setOccupiedTile(0, 0);
					sprites[1]->setCurrentMap(sprites[0]->getCurrentMap());
				}
				if (sprites[2]->getCurrentMap() == Map) {
					ChemistDeltaX = sprites[2]->getPosition()->getX() - x;
					ChemistDeltaY = sprites[2]->getPosition()->getY() - y;
					if (ChemistDeltaX != 0 || ChemistDeltaY != 0) {
						sprites[2]->getCurrentMap()->getMap()[sprites[2]->getPosition()->getRow()][sprites[2]->getPosition()->getCol()] = '.';
						sprites[2]->getWalkEngine()->setOccupiedTile(0, 0);
					}
				} else {
					ChemistDeltaX = 0;
					ChemistDeltaY = 0;
					sprites[2]->getCurrentMap()->getMap()[sprites[2]->getPosition()->getRow()][sprites[2]->getPosition()->getCol()] = '.';
					sprites[2]->getWalkEngine()->setOccupiedTile(0, 0);
					sprites[2]->setCurrentMap(sprites[0]->getCurrentMap());
				}

				if (sprites[3]->getCurrentMap() == Map) {
					PhilosopherDeltaX = sprites[3]->getPosition()->getX() - x;
					PhilosopherDeltaY = sprites[3]->getPosition()->getY() - y;
					if (PhilosopherDeltaX != 0 || PhilosopherDeltaY != 0) {
						sprites[3]->getCurrentMap()->getMap()[sprites[3]->getPosition()->getRow()][sprites[3]->getPosition()->getCol()] = '.';
						sprites[3]->getWalkEngine()->setOccupiedTile(0, 0);
					}
				} else {
					PhilosopherDeltaX = 0;
					PhilosopherDeltaY = 0;
					sprites[3]->getCurrentMap()->getMap()[sprites[3]->getPosition()->getRow()][sprites[3]->getPosition()->getCol()] = '.';
					sprites[3]->getWalkEngine()->setOccupiedTile(0, 0);
					sprites[3]->setCurrentMap(sprites[0]->getCurrentMap());
				}
				firstime = false;
			}

			if (GuardsmanDeltaX != 0 || GuardsmanDeltaY != 0) {
				sprites[1]->getPosition()->moveCardinalDirectionPixel(static_cast<int>(x + GuardsmanDeltaX / 10.0f * counter), y + static_cast<int>(GuardsmanDeltaY / 10.0f * counter), Map);
			}
			if (ChemistDeltaX != 0 || ChemistDeltaY != 0) {
				sprites[2]->getPosition()->moveCardinalDirectionPixel(static_cast<int>(x + ChemistDeltaX / 10.0f * counter), static_cast<int>(y + ChemistDeltaY / 10.0f * counter), Map);
			}
			if (PhilosopherDeltaX != 0 || PhilosopherDeltaY != 0) {
				sprites[3]->getPosition()->moveCardinalDirectionPixel(static_cast<int>(x + PhilosopherDeltaX / 10.0f * counter), static_cast<int>(y + PhilosopherDeltaY / 10.0f * counter), Map);
			}

			AniTimer.Reset();
		}
		if (counter == 0) {
			sprites[1]->setAlive(false);
			sprites[2]->setAlive(false);
			sprites[3]->setAlive(false);
			firstime = true;
			state = 4;
		}
		/*disable gathering change to spreading
		if (toggle == true) {//become opening
			state = 1;
			firstime = true;
			AniTimer.Reset();
		}*/
		break;
	case 4://closed
		if (toggle == false) {
			state = 1;
			AniTimer.Reset();
		}
		break;
	}
}
