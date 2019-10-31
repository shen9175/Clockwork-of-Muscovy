#include <Windows.h>
#include <Wincodec.h>
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
#include <queue>
#include <vector>
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
#pragma comment(lib, "WindowsCodecs.lib")
#pragma comment(lib,"Msimg32.lib")


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow){
	Core* game = new Core(hInstance, hPrevInstance, szCmdLine, iCmdShow);
	int ret = game->run();
	delete game;
	game = nullptr;
	return ret;
}


