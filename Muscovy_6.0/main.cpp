#include <Windows.h>
#include <Wincodec.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
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


