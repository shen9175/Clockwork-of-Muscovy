#include <Wincodec.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <cmath>
#include <queue>
#include <fstream>
#include <string>
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





using namespace std;


Explore::Explore(Core* p, Character* user, Map* &inmap, HBITMAP move[][3], HBITMAP stop[][1], double animationSpeed, string& shoe) : WalkEngine(user, inmap, move, stop, animationSpeed, shoe), pCore(p) {
	hold = false;
	cursorX = user->getPosition()->getX() - user->getCamera()->getLeft();
	cursorY = user->getPosition()->getY() - user->getCamera()->getTop(); 
}

Explore::~Explore() {
}
void Explore::Control() {
	// get input from mouseclick on map.
	if (TargetX == preTargetX && TargetY == preTargetY) {//if last click position and current click position are the same-> either player is in the moving animation or player is idle
														   //update: change to TargetXY from cursorXY, using absolute large map position to avoid click the same window position, player does not move.
		if (pmove != nullptr && !pmove->finish()) {//player is in the moving animation
			//there are at most 3 frames in each animation. if we want these 3 frames finished playing in 0.1s then each frame uses 0.1/3.0 = 0.033 sec per frame
			//each frame consume 10 pixel, 0.1 second moving 3 x 10 = 30 pixel. 1 second moving 30 x 10 = 600 pixel
			if (animationTimer.ElapsedTime() >= animationSpeed) {												
				animationTimer.Reset();
				Update();
			}
		} else {
			//player is in the idle mode, draw the same idle picture
			// idle
			EngineUser->setCurrentSprite(StoppingSprites[EngineUser->getPosition()->getDir()][0]);
			//EngineUser->getCurrentMap()->getShoeSound(shoetype)->Stop();
			if (EngineUser->getPosition()->getX() != TargetX || EngineUser->getPosition()->getY() != TargetY) {//one situation is the current user is temporary blocked by other characters or NPCs ->wait the obstacle move out of the way then re-calculate the new route
				Update();
				delete pmove; //clear the un-finished path if the click happens during the walking
				pmove = new Move(EngineUser, map, TargetX, TargetY);
				pmove->FindPath();
				return;
			}
			if (hold) {//if the mouse left button hold and have not release, then when we finish this path moving, windows system will not sent WM_LBUTTONDOWN message continuely,
					   //so we have to send out fake WM_LBUTTONDOWN message to the wndProc of the current cursor position
				SendMessage(pCore->hChild, WM_LBUTTONDOWN, 0, MAKELPARAM(cursorX, cursorY));//cursorXY never change from last mouse button down, send back to recevie new global large map TargetXY
			}
			if (!movingqueue.empty()) {
					if (movingqueue.front().map == EngineUser->getCurrentMap()->getMapID()) {
						TargetX = movingqueue.front().position.first;
						TargetY = movingqueue.front().position.second;
					}			movingqueue.pop();
			}
		}
	} else {//mouse left button is just clicked and the click points is different from last one
		preTargetX = TargetX;//update clicked position to prev mouse click position
		preTargetY = TargetY;//to let the program re-rount the new path in next round.
		delete pmove; //clear the un-finished path if the click happens during the walking
		pmove = new Move(EngineUser, map, TargetX, TargetY);
		pmove->FindPath();
		//EngineUser->getCurrentMap()->getShoeSound(shoetype)->LoopPlay();
	}
}



void Explore::Update() {

	this->WalkEngine::Update();//call base update() function to save some code

/* battle mode to be continued...
	if (isAround(300, 7, static_cast<int>(pCore->sprites.size()))) // detect enermy; switch to combat mode;
	{
		pCore->mode = 2;//change to the combat mode
		delete pmove;
		pmove = nullptr;
	}*/
	int flag = 0;
	if (!pCore->antiAround.first && (flag = isAround(100, 4, 7, pCore->currentTaskID))) {// detect NPC, switch to dialogue mode;
		if (flag && static_cast<NPC*>(pCore->sprites[flag])->getDialogtrigger()) {
			pCore->mode = 4;//change to the dialogue mode
			ShowWindow(pCore->hwndJornalBT, SW_HIDE);
			ShowWindow(pCore->hwndMapBT, SW_HIDE);
			ShowWindow(pCore->hwndStatusBT, SW_HIDE);
			ShowWindow(pCore->hwndItemListBox, SW_HIDE);
			InvalidateRect(pCore->hwnd, nullptr, true);//clear lower screen picture and buttons 

			//let NPCs turn towards the player by differen direction screnarios
			if (EngineUser->getPosition()->getDir() == 1) {
				pCore->sprites[flag]->setCurrentSprite(StoppingSprites[0][0]);
				pCore->sprites[flag]->getPosition()->setDir(0);
			} else if (EngineUser->getPosition()->getDir() == 0) {
				pCore->sprites[flag]->setCurrentSprite(StoppingSprites[1][0]);
				pCore->sprites[flag]->getPosition()->setDir(1);
			} else if (EngineUser->getPosition()->getDir() == 2) {
				pCore->sprites[flag]->setCurrentSprite(StoppingSprites[3][0]);
				pCore->sprites[flag]->getPosition()->setDir(3);
			} else if (EngineUser->getPosition()->getDir() == 3) {
				pCore->sprites[flag]->setCurrentSprite(StoppingSprites[2][0]);
				pCore->sprites[flag]->getPosition()->setDir(2);
			}

			delete pmove;
			pmove = nullptr;
		}
	}
	if (pCore->antiAround.second != 0) {
		antiAroundfunc(100, pCore->antiAround.second);
	}

}





int Explore::isAround(int d_pixel, int a, int b, int currentTask)
{
	pCore->mobs.clear();
	pCore->n_mobs = 0;
	int r=0;
	int mapRatio = pCore->sprites[a]->getCurrentMap()->getTileSize();
	for (int i = a; i < b; i++)
	{
		//if(this character is avile and in the current map
		if(pCore->sprites[i]->isAlive()&&pCore->sprites[i]->getCurrentMap()->getMapID() == EngineUser->getCurrentMap()->getMapID())
		{
			if (sqrt(pow((float)(pCore->sprites[i]->getPosition()->getY() - EngineUser->getPosition()->getY()),2) + pow((float)(pCore->sprites[i]->getPosition()->getX() - EngineUser->getPosition()->getX()),2)) < d_pixel * mapRatio / 120) 
			{
				if(((NPC*)pCore->sprites[i])->isHostile())
				{
					pCore->mobs.push_back(i);
					pCore->n_mobs++;
				} else {
					if (currentTask == static_cast<NPC*>(pCore->sprites[i])->getTaskID()) {
						return i;
					} else {
						return 0;//if not the task npc, then return 0, as false ->not trigger the dialogue
					}
				}
				++r;
			}

		}
	}

	return r;
}

void Explore::antiAroundfunc(int d_pixel, int i) {
	if (sqrt(pow((float)(pCore->sprites[i]->getPosition()->getY() - EngineUser->getPosition()->getY()),2) + pow((float)(pCore->sprites[i]->getPosition()->getX() - EngineUser->getPosition()->getX()),2)) > d_pixel) 
	{
		pCore->antiAround.first = false;
	}
}



