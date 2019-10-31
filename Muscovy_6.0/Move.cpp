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
#include <fstream>
#include <string>
#include <cmath>
#include <queue>
#include <ctime>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "pqastar.h"
#include "astar.h"
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


fstream outfile;

Move::Move( Character* sprite, Map* &inmap, int x, int y) : player(sprite), map(inmap), MouseX(x), MouseY(y) {
path = nullptr;
movingStatus = 1;
FrameCounter = 0;
finishMoving = true;
currRow = 0;
currCol = 0;
nextRow = 0;
nextCol = 0;
RowMovePiece = 0;
ColMovePiece = 0;
twoPartsMovingTargetX = 0;
twoPartsMovingTargetY = 0;
TwoPartsMovingfirstHalf = 0;
twoPartsMoving1stVertexX = 0;
twoPartsMoving1stVertexY = 0;
twoPartsMoving2ndVertexX = 0;
twoPartsMoving2ndVertexY = 0;
}

Move::~Move() {
	while (path != nullptr) {
		nodeptr temp = path;
		path = path->next;
		delete temp;
		temp = nullptr;
	}
}

void Move::MoveControl() {

	if (path != nullptr) {
		switch (movingStatus) {
		case 1:
			NormalMoving();
			break;
		case 2:
			TwoPartsMoving();
			break;
		case 3:
			FinalPixelDelivery();
			break;
		default:
			break;
		}
	}
}

void Move::NormalMoving() {

	if (FrameCounter % map->getMovingSteps() == 0) {
		FrameCounter = 0;
		if (path == nullptr) {
			MessageBox(nullptr, "Assert: path is nullptr in NormalMoving", "Wrong", MB_ICONEXCLAMATION | MB_OK);
			exit(-1);
		}
		if (path->next == nullptr) {//path->next == nullptr --> finishing moving in normal moving since FrameCounter % step == 0)
			RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
			ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
			movingStatus = 3;
			return;
		} else {  //path->next != nullptr: at least still have 2+ tiles--> at least 1+ big moves, starting from here: FrameCounter % step == 0->just finish last big move, start a new big move
			if (path->next->next == nullptr) {//path->next != nullptr since it's in the path->next == nullptr else statement)
				currRow = path->info.y;
				currCol = path->info.x;
				nodeptr temp = path;
				path = path->next;
				nextRow = path->info.y;
				nextCol = path->info.x;
				int deltaRow = nextRow - currRow;
				int deltaCol = nextCol - currCol;
				delete temp;
				temp = nullptr;
				bool whichmove = whichmoving(player->getPosition()->getX(), player->getPosition()->getY(), deltaRow, deltaCol);
				if (whichmove) {
					RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
					ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
					movingStatus = 3;
					return;
				} else {
					movingStatus = 2;
					RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
					ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
					TwoPartsMovingfirstHalf = true;//two parts moving final destination is the current player's position parallel moving to the next tile's same position
					twoPartsMovingTargetX = MouseX;//the second half final target become final pixel delivery
					twoPartsMovingTargetY = MouseY;//the second half final target become final pixel delivery
					return;
				}
			} else {//there are more than two tiles to go. try normal or two parts moving.
				currRow = path->info.y;
				currCol = path->info.x;
				nodeptr temp = path;
				path = path->next;
				nextRow = path->info.y;
				nextCol = path->info.x;
				int deltaRow = nextRow - currRow;
				int deltaCol = nextCol - currCol;
				delete temp;
				temp = nullptr;
				bool whichmove = whichmoving(player->getPosition()->getX(), player->getPosition()->getY(), deltaRow, deltaCol);
				if (whichmove) { //normal moving
					RowMovePiece = deltaRow * map->getTileSize() / map->getMovingSteps();
					ColMovePiece = deltaCol * map->getTileSize() / map->getMovingSteps();
				} else {//two parts moving
					movingStatus = 2;
					RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
					ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
					TwoPartsMovingfirstHalf = true;//two parts moving final destination is the current player's position parallel moving to the next tile's same position
					twoPartsMovingTargetX = player->getPosition()->getX() + deltaCol * map->getTileSize();
					twoPartsMovingTargetY = player->getPosition()->getY() + deltaRow * map->getTileSize();
				}
			}
		}
	}
}

void Move::TwoPartsMoving() {
	int X = player->getPosition()->getX();
	int Y = player->getPosition()->getY();
	int EachStepPixels = map->getTileSize() / map->getMovingSteps();
	int deltaRow;
	int deltaCol;
	double hypotenuse;
	if (TwoPartsMovingfirstHalf) {
		if (Y == twoPartsMoving1stVertexY && X == twoPartsMoving1stVertexX) {//if reach the first tile destination vertex
			TwoPartsMovingfirstHalf = false;//change status to the second half
			if (map->getMap()[twoPartsMoving2ndVertexY / map->getTileSize()][twoPartsMoving2ndVertexX / map->getTileSize()] == 'w' ){//next tile is occupied by others(must be others not self), clear path and return
				while (path != nullptr) {
					nodeptr temp = path;
					path = path->next;
					delete temp;
					temp = nullptr;
				}
				movingStatus = 1;
				FrameCounter = 0;
				finishMoving = true;
				RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
				ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
				return;
			}
			player->getCurrentMap()->getMap()[player->getPosition()->getRow()][player->getPosition()->getCol()] = '.';//clear the occupied tile
			player->getPosition()->moveCardinalDirectionPixel(twoPartsMoving2ndVertexX, twoPartsMoving2ndVertexY, map);//move 1 pixel to next tile starting pixel.The second tile starting vertex only used here.
			player->getCurrentMap()->getMap()[player->getPosition()->getRow()][player->getPosition()->getCol()] = 'w';//occupy the next tile
			player->getWalkEngine()->setOccupiedTile(player->getPosition()->getRow(), player->getPosition()->getCol());//record the current self possessed 'w'
			return;
		} else {//if has not reached the first tile destination vertex
			deltaRow = twoPartsMoving1stVertexY - Y;//keep recalculating the difference between current player pixels and the first tile destination vertex pixels
			deltaCol = twoPartsMoving1stVertexX - X;
			hypotenuse = sqrt(deltaRow * deltaRow + deltaCol * deltaCol);//keep recalculating the hypotenuse between these two points
		}
	} else {
		if (Y == twoPartsMovingTargetY && X == twoPartsMovingTargetX) {//if reach the second tile destination points
			movingStatus = 1;//finishing this two part moving, and go back to normal moving
			TwoPartsMovingfirstHalf = true;//switch to FristHalf for next two parts moving
			FrameCounter = 0;//since no matter which moving it takes, FrameCounter will increment. But only Normal moving need it. So reset it for the new round of normal moving
			RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
			ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
			return;
		} else {//if has not reached the second tile destination points
			deltaRow = twoPartsMovingTargetY - Y;//keep recalculating the difference between current player pixels and the second tile destination points
			deltaCol = twoPartsMovingTargetX - X;
			hypotenuse = sqrt(deltaRow * deltaRow + deltaCol * deltaCol);//keep recalculating the hypotenuse between these two points
		}
	}

	if (hypotenuse <= EachStepPixels * 2) {//if the linear distance(hypotenuse) is smaller than 2 * 10 pixel, move the player directly to the destination since it's 2 parts, so each part double the speed
		RowMovePiece = deltaRow;
		ColMovePiece = deltaCol;
	} else {//if larger than 10 pixels or other define distance, change the vertical and horizontal distance proportionally which make the player moving along the hypotenuse (shortest parth in a tile)
		RowMovePiece = static_cast<int>(deltaRow / hypotenuse * EachStepPixels * 2);
		ColMovePiece = static_cast<int>(deltaCol / hypotenuse * EachStepPixels * 2);
	}
}
void Move::FinalPixelDelivery() {
	//same idea as two part moving.
	int X = player->getPosition()->getX();
	int Y = player->getPosition()->getY();
	int EachStepPixels = map->getTileSize() / map->getMovingSteps();
	int deltaRow;
	int deltaCol;
	double hypotenuse;

	//the secondary A-start search could produce a one-tile node with the content of the player's current position(starting) tile if the player next to the 'w'
	//Then it will be into the FinalPixelDelivery part(only one node left), and this part will send player directly to the final mouse click points, no matter the tiles passed by are 'w' tiles
	//in this case the player will be in the 'w' tile (since it went through the second astart search, it means the player click on a 'w' tile as the final delivery point)
	//but this 'w' is not the player self's 'w'
	int temprow = MouseY / map->getTileSize();
	int tempcol = MouseX / map->getTileSize();
	if (map->getMap()[temprow][tempcol] == 'w' && (temprow != player->getWalkEngine()->getOccupiedTile().first || tempcol != player->getWalkEngine()->getOccupiedTile().second)) {
		delete path;
		path = nullptr;
		movingStatus = 1;
		FrameCounter = 0;
		finishMoving = true;
		RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
		ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
		//
		//MouseX = player->getPosition()->getCol();
		//MouseY = player->getPosition()->getRow();
		return;
	}
	if (Y == MouseY && X == MouseX) {
		delete path;
		path = nullptr;
		movingStatus = 1;
		FrameCounter = 0;
		finishMoving = true;
		RowMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
		ColMovePiece = 0;//it will go back to update Row/ColMovePiece one more time before change the moving types, so need clear these two values.
		//return;
	} else {
		deltaRow = MouseY - Y;
		deltaCol = MouseX - X;
		hypotenuse = sqrt(deltaRow * deltaRow + deltaCol * deltaCol);
		if (hypotenuse <= EachStepPixels) {
			RowMovePiece = deltaRow;
			ColMovePiece = deltaCol;
		} else {
			RowMovePiece = static_cast<int>(deltaRow / hypotenuse * EachStepPixels);
			ColMovePiece = static_cast<int>(deltaCol / hypotenuse * EachStepPixels);
		}
	}
}

bool Move::whichmoving(int X, int Y, int deltaRow, int deltaCol) {
	bool ret = true;
	if (X >= currCol * map->getTileSize() && X < (currRow + currCol) * map->getTileSize() + map->getTileSize() - 1 - Y
		&& Y >= currRow * map->getTileSize() && Y < (currRow + currCol) * map->getTileSize() + map->getTileSize() - 1 - X) {
		if (deltaRow == 1 && deltaCol == -1) {//start from right top to left bottom
			if (map->getMap()[currRow][nextCol] == 'w') {//left top is 'w' not matter if right bottom is or is not 'w'
				//two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize();
				twoPartsMoving1stVertexY = currRow * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize();
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else { //left top is not 'w' no matter right bottom is or is not 'w'
				//normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}

		} else if (deltaRow == -1 && deltaCol == 1) {//start from left bottom to right top
			if (map->getMap()[nextRow][currCol] == 'w') {//left top is 'w'
				//two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving1stVertexY = currRow * map->getTileSize();
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize();
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize() + map->getTileSize() - 1;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else { //left top is not 'w' no matter right bottom is or is not 'w'
				 //normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		} else {
			;
			//The other direction cases are talking in the under secondary diagonal part and primary diagonal parts. This above secondary diagonal does not determine other directions
			//So when talk to the primary diagonal, use "if" not "else if" statement or it will be determined for normal moving only in secondary diagonal part.
		}
	} else if (X > (currRow + currCol) * map->getTileSize() + map->getTileSize() - 1 - Y && X <= map->getTileSize() * currCol + map->getTileSize() - 1
		&& Y > (currRow + currCol) * map->getTileSize() + map->getTileSize() - 1 - X && Y <= map->getTileSize() * currRow + map->getTileSize() - 1) {
		//the player is under secondary diagonal
		if (deltaRow == 1 && deltaCol == -1) {//start from right top to left bottom
			if (map->getMap()[nextRow][currCol] == 'w') {//right bottom is 'w' no matter left top is or is not 'w'
				//two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize();
				twoPartsMoving1stVertexY = currRow * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize();
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else {//right bottom is not 'w' no matter left top is or is not 'w'
				//normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		} else if (deltaRow == -1 && deltaCol == 1) {//start from left bottom to right top
			if (map->getMap()[currRow][nextCol] == 'w') {//right bottom is 'w' no matter left top is or is not 'w'
			   //two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving1stVertexY = currRow * map->getTileSize();
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize();
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize() + map->getTileSize() - 1;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else {//right bottom is not 'w' no matter left top is or is not 'w'
				//normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		} else {
			;
			//The other direction cases are talking in the primary diagonal parts. This under secondary diagonal does not determine other directions
			//So when talk to the primary diagonal, use "if" not "else if" statement or it will be determined for normal moving only in secondary diagonal part.
		}
	} else if (X == (currRow + currCol) * map->getTileSize() + map->getTileSize() - 1 - Y
		&& Y == (currRow + currCol) * map->getTileSize() + map->getTileSize() - 1 - X) {
		//the player is on the secondary diagonal
		if ((deltaRow == 1 && deltaCol == -1) || (deltaRow == -1 && deltaCol == 1)) {
			//normal moving
			ret = true;
			return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
		} else {
			//to be determined on the primary diagonal part
		}
	} else {
		MessageBox(nullptr, "Wrong in which moving secondary diagonal part!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	//primary diagonal verification should start a new if statement, not use else if connect to above secondary diagonal verification.
	//Or you got the player above the secondary diagonal, 'w' is not on your way, that not means there is no 'w' on your way, only right top to left bottom and left bottom to right top has been checked
	//you have to to check left top to right bottom and right bottom to left top which is mainly affected by primary diagonal
	if (X > Y - map->getTileSize() * (currRow - currCol) && X <= currCol * map->getTileSize() + map->getTileSize() - 1
		&& Y < X + map->getTileSize() * (currRow - currCol) && Y >= currRow * map->getTileSize()) {
		//the player is on the primary diagonal
		if (deltaRow == 1 && deltaCol == 1) { //start from left top to right bottom
			if (map->getMap()[currRow][nextCol] == 'w') {//right top is 'w' no matter left bottom is or is not 'w'
				// two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving1stVertexY = currRow * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize();
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize();
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else {//right top is no 'w' no matter left bottom is or is not 'w'
				// normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		} else if (deltaRow == -1 && deltaCol == -1) {//start from right bottom to left top
			if (map->getMap()[nextRow][currCol] == 'w') {//right top is 'w' no matter left bottom is or is not 'w'
			   //two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize();
				twoPartsMoving1stVertexY = currRow * map->getTileSize();
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize() + map->getTileSize() - 1;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else {//right top is not 'w' no matter left bottom is or is not 'w'
				//normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		}
		else {
			ret = true;
			return ret;
		}
	} else if (X < Y - map->getTileSize() * (currRow - currCol) && X >= currCol * map->getTileSize()
		&& Y > X + map->getTileSize() * (currRow - currCol) && Y <= currRow * map->getTileSize() + map->getTileSize() - 1) {
		//the player is under the primary diagonal
		if (deltaRow == 1 && deltaCol == 1) { //start from left top to right bottom
			if (map->getMap()[nextRow][currCol] == 'w') {//left bottom is 'w' no matter right top is or is not 'w'
			   // two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving1stVertexY = currRow * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize();
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize();
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else {//left bottom is not 'w' no matter right top is or is not 'w'
				// normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		} else if (deltaRow == -1 && deltaCol == -1) {//start from right bottom to left top
			if (map->getMap()[currRow][nextCol] == 'w') {//left bottom is 'w' no matter right top is or is not 'w'
				//two parts moving
				ret = false;
				twoPartsMoving1stVertexX = currCol * map->getTileSize();
				twoPartsMoving1stVertexY = currRow * map->getTileSize();
				twoPartsMoving2ndVertexX = nextCol * map->getTileSize() + map->getTileSize() - 1;
				twoPartsMoving2ndVertexY = nextRow * map->getTileSize() + map->getTileSize() - 1;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			} else {//left bottom is not 'w' no matter right top is or is not 'w'
				//normal moving
				ret = true;
				return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
			}
		}
	} else if (X == Y - map->getTileSize() * (currRow - currCol) && X >= currCol * map->getTileSize()
		&& Y == X + map->getTileSize() * (currRow - currCol) && Y <= currRow * map->getTileSize() + map->getTileSize() - 1) {
		if ((deltaRow == 1 && deltaCol == 1) || (deltaRow == -1 && deltaCol == -1)) {
			//normal moving
			ret = true;
			return ret;//need return immediately since it will go though the other diagonal. If do not return, it will overwrite by finally true since it will not any of other cases
		} else {
			ret = true;
			return ret;
		}

	} else {
		MessageBox(nullptr, "Wrong in which moving primary diagonal part!", "Wrong", MB_ICONEXCLAMATION | MB_OK);
	}
	return ret;
}

void Move::FindPath() {
	map->getMap()[player->getWalkEngine()->getOccupiedTile().first][player->getWalkEngine()->getOccupiedTile().second] = '.';//resume the current possessed tile for astar search
	path = astar1_search(map->getMap(),  map->getTileHeight(), map->getTileWidth(), MouseY / map->getTileSize(), MouseX / map->getTileSize(), player->getPosition()->getRow(), player->getPosition()->getCol());
	map->getMap()[player->getWalkEngine()->getOccupiedTile().first][player->getWalkEngine()->getOccupiedTile().second] = 'w';//repossess the current tile after astar search
	if (path != nullptr && (path->info.x != player->getPosition()->getCol() || path->info.y != player->getPosition()->getRow())) {
		MessageBox(0, "player position is not same with astar starting point!", "first part", MB_ICONEXCLAMATION | MB_OK);
	}
	/***********************************************/
	/*                                             */
	/* The following code is let the player walk   */
	/* towads the target as close as possible, the */
	/* player will reach the closest possible place*/
	/* near the target. (Better solution)          */
	/*                                             */
	/***********************************************/
	int newGoalX = MouseX / map->getTileSize(); //let the newGoalXY first be the target in the large map
	int newGoalY = MouseY / map->getTileSize();
	//Start from mouse pointed tile search backward on the line to the player current position, find the first non obstacle tile then try astar search, if still no rount out,
	//keep search backward, until find a rount out. There must be a tile could go unless 8 obstacle surround the player, in this way, path should be nullptr from first astart search.
	while (path == nullptr && (newGoalX != player->getPosition()->getCol() || newGoalY != player->getPosition()->getRow())) {
		if (map->getMap()[newGoalY][newGoalX] != 'w') {// even the new attempt tile is not 'w' but still no route out (new attempt is surrounded by 'w's) try next liner positon toward the player
			if (newGoalY < player->getPosition()->getRow()) {
				++newGoalY;
			}
			else if (newGoalY > player->getPosition()->getRow()) {
				--newGoalY;
			}
			if (newGoalX < player->getPosition()->getCol()) {
				++newGoalX;
			}
			else if (newGoalX > player->getPosition()->getCol()) {
				--newGoalX;
			} else {//newGoalXY = current position, the astar should have at least one node whichi is current position, should not need this else, put here just in case.
				break;
			}
		} else {
			while (newGoalY >= 0 && newGoalY < map->getTileHeight() &&	//keep try to find the first none 'w' place in the line between target and player-->(staring from target),
				newGoalX >= 0 && newGoalX < map->getTileWidth() &&      //if the new found one still cannot be rounted to the player, keep finding next none 'w' in this line with this pattern,
				map->getMap()[newGoalY][newGoalX] == 'w') {							//until found one or finally to be the player itself.
				if (newGoalY < player->getPosition()->getRow()) {
					++newGoalY;
				} else if (newGoalY > player->getPosition()->getRow()) {
					--newGoalY;
				}
				if (newGoalX < player->getPosition()->getCol()) {
					++newGoalX;
				} else if (newGoalX > player->getPosition()->getCol()) {
					--newGoalX;
				} else {//newGoalXY = current position
					break;
				}
			}
		}
		map->getMap()[player->getWalkEngine()->getOccupiedTile().first][player->getWalkEngine()->getOccupiedTile().second] = '.';//resume the current possessed tile for astar search
		path = astar1_search(map->getMap(),  map->getTileHeight(), map->getTileWidth(), newGoalY, newGoalX, player->getPosition()->getRow(), player->getPosition()->getCol());
		map->getMap()[player->getWalkEngine()->getOccupiedTile().first][player->getWalkEngine()->getOccupiedTile().second] = 'w';//repossess the current tile after astar search
	}
	if (path != nullptr && (path->info.x != player->getPosition()->getCol() || path->info.y != player->getPosition()->getRow())) {
		MessageBox(0, "player position is not same with astar starting point!", "second part", MB_ICONEXCLAMATION | MB_OK);
	}
	//the new GoalXY will(must) ultimately be found: the wrost case, it will be player itself.
	if (path != nullptr) {
		finishMoving = false;
	}
}