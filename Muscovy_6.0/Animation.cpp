#include <Windows.h>
#include <ctime>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#if defined(_MSC_VER)
#include <al.h>
#include <alc.h>
#include <efx.h>
#include <alut.h>
#elif defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/efx.h>
#include <OpenAL/alut.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/alut.h>
#endif
#include <unordered_map>
#include <unordered_set>
#include "timer.h"
#include "Sound.h"
#include "Map.h"
#include "Camera.h"
#include "Animation.h"


Animation::Animation(HBITMAP sprites[][3], int directions, int frames, float timeTofinish) :  animation(sprites), orientation(directions), totalframes(frames), time(timeTofinish) {
	counter = 0;
	currentFrame = 0;
	oneTimeDrawLatch = false;
	currentDirection = 0;
}
Animation::~Animation() {

}

void Animation::Update() {
	if (animationTimer.ElapsedTime() > time / totalframes) {
		++counter;
		animationTimer.Reset();
		currentFrame = counter % totalframes;
	}
}
void Animation::oneTimeDrawStart(int x, int y, int w, int h, Camera* cam) {
	oneTimeDrawLatch = true;
	animationTimer.Reset();
	oneTimeDraw_x = x;
	oneTimeDraw_y = y;
	oneTimeDraw_w = w;
	oneTimeDraw_h = h;
	counter = 0;
	camera = cam;
	currentFrame = 0; 
}
void Animation::oneTimeDraw(HDC hdcMem) {
	if (oneTimeDrawLatch) {
		if (animationTimer.ElapsedTime() > time / totalframes) {
			animationTimer.Reset();
			++counter;//when mouse click calls oneTimeDrawStart(), it enable oneTimeDrawLatch, and when it comes here, it already display for a period of time/totalframes time for animation[dir][0], here we need to display animation[dir][1]
			currentFrame = counter;
			if (counter == totalframes) {//first ++counter before currentFrame = counter, let the last frame display for another time/totalframes period of time. or it will display only for milisecond and gone.
				oneTimeDrawLatch = false;
			}
		}
		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 255;
		bf.AlphaFormat = AC_SRC_ALPHA;
		HDC hdcSquare = CreateCompatibleDC(hdcMem);
		HBITMAP oldsprites = (HBITMAP)SelectObject(hdcSquare, animation[currentDirection][currentFrame]);
		BITMAP bitmaptemp;
		GetObject(animation[currentDirection][currentFrame], sizeof(bitmaptemp), &bitmaptemp);
		int tempx = oneTimeDraw_x - camera->getLeft() - oneTimeDraw_w / 2;
		int tempy = oneTimeDraw_y - camera->getTop() - oneTimeDraw_h / 2;
		AlphaBlend(hdcMem, oneTimeDraw_x - camera->getLeft() - oneTimeDraw_w / 2, oneTimeDraw_y - camera->getTop() - oneTimeDraw_h / 2, oneTimeDraw_w, oneTimeDraw_h, hdcSquare, 0, 0, bitmaptemp.bmWidth, bitmaptemp.bmHeight, bf);
		SelectObject(hdcSquare, oldsprites);
		DeleteDC(hdcSquare);
	}
}