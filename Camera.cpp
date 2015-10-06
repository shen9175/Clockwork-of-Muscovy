#include <algorithm>
#include <ctime>
#include <Windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "timer.h"
#include "Sound.h"
#include "game.h"
#include "Map.h"
#include "Camera.h"
using namespace std;

Camera::Camera(int MapWidth, int MapHeight, int playerX, int playerY) {
		left = max(0, playerX - ResX / 2);
		if (left == 0) {
			right = ResX;
		} else {
			right = min(MapWidth, playerX + ResX / 2);
			if (right == MapWidth) {
				left = MapWidth - ResX;
			}
		}
		top = max(0, playerY - ResY / 2);
		if (top == 0) {
			bottom = ResY;
		} else {
			bottom = min(MapHeight, playerY + ResY / 2);
			if (bottom == MapHeight) {
				top = MapHeight - ResY;
			}
		}
		
	/* it's not that easy as follows
		left = max(0, playerX - ResX / 2);
		right = min(MapWidth, playerX + ResX / 2);
		top = max(0, playerY - ResY / 2);
		bottom = min(MapHeight, playerY + ResY / 2);
		*/
}
Camera::Camera(Map* map, int playerX, int playerY) {
	left = max(0, playerX - ResX / 2);
	if (left == 0) {
		right = ResX;
	} else {
		right = min(map->getPixelWidth(), playerX + ResX / 2);
		if (right == map->getPixelWidth()) {
			left = map->getPixelWidth() - ResX;
		}
	}
	top = max(0, playerY - ResY / 2);
	if (top == 0) {
		bottom = ResY;
	} else {
		bottom = min(map->getPixelHeight(), playerY + ResY / 2);
		if (bottom == map->getPixelHeight()) {
			top = map->getPixelHeight() - ResY;
		}
	}
}
void Camera::setCamera(int MapWidth, int MapHeight, int playerX, int playerY) {
	left = max(0, playerX - ResX / 2);
	if (left == 0) {
		right = ResX;
	} else {
		right = min(MapWidth, playerX + ResX / 2);
		if (right == MapWidth) {
			left = MapWidth - ResX;
		}
	}
	top = max(0, playerY - ResY / 2);
	if (top == 0) {
		bottom = ResY;
	} else {
		bottom = min(MapHeight, playerY + ResY / 2);
		if (bottom == MapHeight) {
			top = MapHeight - ResY;
		}
	}
}
void Camera::setCamera(Map* map, int playerX, int playerY) {
	left = max(0, playerX - ResX / 2);
	if (left == 0) {
		right = ResX;
	} else {
		right = min(map->getPixelWidth(), playerX + ResX / 2);
		if (right == map->getPixelWidth()) {
			left = map->getPixelWidth() - ResX;
		}
	}
	top = max(0, playerY - ResY / 2);
	if (top == 0) {
		bottom = ResY;
	} else {
		bottom = min(map->getPixelHeight(), playerY + ResY / 2);
		if (bottom == map->getPixelHeight()) {
			top = map->getPixelHeight() - ResY;
		}
	}
}

void Camera::CameraUpdate(int MapWidth, int MapHeight, int playerX, int playerY, int deltaX, int deltaY) {
	if(((playerX + deltaX) > ResX / 2) && (playerX + deltaX) <= (MapWidth - ResX / 2)) {
		left += deltaX;
		right += deltaX;
	}
	if(((playerY + deltaY) > ResY / 2) && (playerY + deltaY) <= (MapHeight - ResY / 2)) {
		static int delta = 300;
		top += deltaY;
		bottom += deltaY;
		if (playerY - top < delta) {
			delta = playerY - top;
		}
	}

	//it's not as easy as follows: play if is not in the center of the screen, playerX - ResX / 2 is not the left
	/*	left = max(0, playerX + deltaX - ResX / 2);
	right = min(MapWidth, playerX + deltaX + ResX / 2);
	top = max(0, playerY + deltaY  - ResY / 2);
	bottom = min(MapHeight, playerY + deltaY + ResY / 2);*/
}