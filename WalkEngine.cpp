#include <string>
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



WalkEngine::WalkEngine(Character* user, Map* &inmap, HBITMAP moving[][3], HBITMAP stopping[][1], double aniSpeed, string& shoe) : EngineUser(user), map(inmap),MovingSprites(moving), StoppingSprites(stopping), animationSpeed(aniSpeed), shoetype(shoe) {
	TargetX = user->getPosition()->getX();
	TargetY = user->getPosition()->getY();
	preTargetX = user->getPosition()->getX();
	preTargetY = user->getPosition()->getY();
	occupiedTile = {0,0};
	animationTimer.Reset();
	footstepSoundTimer.Reset();
	footSpeedTimer.Reset();
	animationPic = 0;
}

void WalkEngine::resetWalkEngine() {
	delete pmove;
	pmove = nullptr;
	TargetX = EngineUser->getPosition()->getX();
	TargetY = EngineUser->getPosition()->getY();
	preTargetX = EngineUser->getPosition()->getX();
	preTargetY = EngineUser->getPosition()->getY();
	occupiedTile = {0,0};
	animationTimer.Reset();
	footstepSoundTimer.Reset();
	footSpeedTimer.Reset();
	animationPic = 0;
	queue<TARGET> empty;
	swap(movingqueue, empty);
}


void WalkEngine::Control() {
	if (TargetX == preTargetX && TargetY == TargetY) {//if last position and current position are the same-> either player is in the moving animation or player is idle
														   //update: change to MouseXY from cursorXY, using absolute large map position to avoid click the same window position, player does not move.
		if (pmove != nullptr && !pmove->finish()) {//player is in the moving animation
												   //there are at most 3 frames in each animation. if we want these 3 frames finished playing in 0.1s then each frame uses 0.1/3.0 = 0.033 sec per frame
												   //each frame consume 10 pixel, 0.1 second moving 3 x 10 = 30 pixel. 1 second moving 30 x 10 = 600 pixel
			if (animationTimer.ElapsedTime() >= animationSpeed) {												
				animationTimer.Reset();
				Update();
			}
		} else {
			//EngineUser->getCurrentMap()->getShoeSound(shoetype)->Stop();
			// idle:  player is in the idle mode, draw the same idle picture
			EngineUser->setCurrentSprite(StoppingSprites[EngineUser->getPosition()->getDir()][0]);
			if (EngineUser->getPosition()->getX() != TargetX || EngineUser->getPosition()->getY() != TargetY) {//one situation is the current user is temporary blocked by other characters or NPCs ->wait the obstacle move out of the way then re-calculate the new route
				delete pmove; //clear the un-finished path if the click happens during the walking
				pmove = new Move(EngineUser, map, TargetX, TargetY);
				pmove->FindPath();
				//EngineUser->getCurrentMap()->getShoeSound(shoetype)->LoopPlay();
				return;
			}
			if (!movingqueue.empty()) {
				if (movingqueue.front().map == EngineUser->getCurrentMap()->getMapID()) {
					TargetX = movingqueue.front().position.first;
					TargetY = movingqueue.front().position.second;
				}
				movingqueue.pop();
			} else {
				static_cast<NPC*>(EngineUser)->setDialogtrigger(true);//only stop can accept dialog with user.
			}
		}
	}
	else {//mouse left button is just clicked and the click points is different from last one

		preTargetX = TargetX;//update clicked position to prev mouse click position
		preTargetY = TargetY;//to let the program re-rount the new path in next round.
		delete pmove; //clear the un-finished path if the click happens during the walking
		pmove = new Move(EngineUser, map, TargetX, TargetY);
		pmove->FindPath();
		//EngineUser->getCurrentMap()->getShoeSound(shoetype)->LoopPlay();
	}

}
void WalkEngine::Update() {
	if (pmove == nullptr) {//this is only in waiting for new rount mode that pmove has been delete by termination of current route, call this update only use detect NPC
		return;
	}
	pmove->MoveControl();
	int RowPiece = pmove->getVerticalMovementPiece();
	int ColPiece = pmove->getHorizontalMovementPiece();
	//update user's internal coordinates
	
	int futureCol = (EngineUser->getPosition()->getX() + ColPiece) / map->getTileSize();
	int futureRow = (EngineUser->getPosition()->getY() + RowPiece) / map->getTileSize();

	if ((futureCol != EngineUser->getPosition()->getCol() || futureRow != EngineUser->getPosition()->getRow()) 
		&& (map->getMap()[futureRow][futureCol] == 'w') 
		&& (futureRow != EngineUser->getWalkEngine()->getOccupiedTile().first || futureCol != EngineUser->getWalkEngine()->getOccupiedTile().second)) { //if except the current cell and the next cell is ocuppied by other sprites, just set current position as start and re-route the path
		preTargetX = EngineUser->getPosition()->getX();
		preTargetY = EngineUser->getPosition()->getY();
		delete pmove;
		pmove = nullptr;
		return;
	}

	if (futureCol != EngineUser->getPosition()->getCol() || futureRow != EngineUser->getPosition()->getRow()) {//if the user step into a new tile
		map->getMap()[futureRow][futureCol] = 'w';//occupy the new tile
		occupiedTile = {futureRow, futureCol};
		map->getMap()[EngineUser->getPosition()->getRow()][EngineUser->getPosition()->getCol()] = '.';//release the old tile for others
	}
	//must update camera first since camera adjustment is based on original player's position not the updated player's position
	EngineUser->getCamera()->CameraUpdate(map->getPixelWidth(), map->getPixelHeight(), EngineUser->getPosition()->getX(), EngineUser->getPosition()->getY(), ColPiece, RowPiece);
	EngineUser->getPosition()->moveCardinalDirectionPixel(EngineUser->getPosition()->getX() + ColPiece, EngineUser->getPosition()->getY() + RowPiece, map);
	
	Sound* currentSound = EngineUser->getCurrentMap()->getShoeSound(shoetype);
	double elaspedtime = footstepSoundTimer.ElapsedTime();
	if (elaspedtime > 0.3 && (RowPiece != 0 || ColPiece != 0)) {
		footstepSoundTimer.Reset();
		currentSound->X3DPositionalSoundCalculation(
			EngineUser->getpCore()->sprites[0]->getCamera()->getLeft() + (EngineUser->getpCore()->sprites[0]->getCamera()->getRight() - EngineUser->getpCore()->sprites[0]->getCamera()->getLeft()) / 2.0f,
			EngineUser->getpCore()->sprites[0]->getCamera()->getTop() + (EngineUser->getpCore()->sprites[0]->getCamera()->getBottom() - EngineUser->getpCore()->sprites[0]->getCamera()->getTop()) / 2.0f,
			200.0f, 
			static_cast<float>(EngineUser->getPosition()->getX()), 
			static_cast<float>(EngineUser->getPosition()->getY()), 
			0.0f, 
			static_cast<float>(elaspedtime)
			);
		currentSound->Play();
	}

	if (ColPiece != 0 && RowPiece != 0) {
		if (abs(ColPiece) > abs(RowPiece) / 4) {//4 is just a number to tweak the change of walking horizontally and vertically. Here this number the larger, the more change of walking right/left than up/down
			if (ColPiece > 0) {// move right;
				EngineUser->setCurrentSprite(MovingSprites[3][animationPic % 3]);
				EngineUser->getPosition()->setDir(3);
			} else { // move left;
				EngineUser->setCurrentSprite(MovingSprites[2][animationPic % 3]);
				EngineUser->getPosition()->setDir(2);
			}
		} else {
			if (RowPiece > 0) { // move down;
				EngineUser->setCurrentSprite(MovingSprites[1][animationPic % 3]);
				EngineUser->getPosition()->setDir(1);
			} else { // move up
				EngineUser->setCurrentSprite(MovingSprites[0][animationPic % 3]);
				EngineUser->getPosition()->setDir(0);
			}
		}
	} else if (ColPiece == 0 && RowPiece == 0) {
		EngineUser->setCurrentSprite(StoppingSprites[EngineUser->getPosition()->getDir()][0]);
	} else {
		if (ColPiece > 0) {// move right;
			EngineUser->setCurrentSprite(MovingSprites[3][animationPic % 3]);
			EngineUser->getPosition()->setDir(3);
		} else if (ColPiece < 0) { // move left;
			EngineUser->setCurrentSprite(MovingSprites[2][animationPic % 3]);
			EngineUser->getPosition()->setDir(2);
		} else if (RowPiece > 0) { // move down;
			EngineUser->setCurrentSprite(MovingSprites[1][animationPic % 3]);
			EngineUser->getPosition()->setDir(1);
		} else if (RowPiece < 0) { // move up
			EngineUser->setCurrentSprite(MovingSprites[0][animationPic % 3]);
			EngineUser->getPosition()->setDir(0);
		}//else ColPiece and RowPiece == 0, stop, do not change direction
	}
	animationPic++;
	pmove->incrementFrame();
	//to see if current pixel point entering the map switch trigger rectangle
	Map* temp = EngineUser->getCurrentMap()->MapSwitch(EngineUser->getPosition()->getX(), EngineUser->getPosition()->getY(),EngineUser);
	if ( temp != nullptr) {//MapSwitch will sent the current character to the next map, here don't need update the character's coordinates and camera
						//it has to update in the mapswitch, because it needs the corresponding door's initial position, they get it can not pass it here.
		delete pmove;
		pmove = nullptr;
		// update player's current map is done in mapswich
	}
}
