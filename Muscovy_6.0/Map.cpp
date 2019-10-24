#include <Windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <vector>
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <queue>
#include "Sound.h"
#include "Map.h"
#include "Camera.h"
#include "pqastar.h"
#include "timer.h"
#include "Move.h"
#include "WalkEngine.h"
#include "Explore.h"
#include "character.h"

using namespace std;


Map::Map(int ID, string NAME, int tile, int steps, int w, int h, string filepath, SoundDevice* pSoundDevice, string bgm, unordered_map<string, Sound*>* shoeSoundpair, float& gVolume) : mapID(ID) ,name(NAME), tile_size(tile), step(steps), width(w), height(h), mapfile_path(filepath), BGM(new Sound(pSoundDevice,bgm)), shoeSound(shoeSoundpair), volume(gVolume) {
	ifstream in_file;
	string line;
	in_file.open(mapfile_path.c_str());
	if (in_file.fail()) {
		MessageBox(NULL, "Open failed!", mapfile_path.c_str(), MB_ICONEXCLAMATION | MB_OK);
		exit(-1);
	}
	map = new char*[height];
	for(int i = 0; i < height; i++) {
		map[i] = new char[width];
	}

	for (int i=0; i < height; i++) {
		in_file >> line;
		for(int j = 0; j < width; j++) {
			map[i][j] = line[j];
		}
	}
}
Map::~Map() {
	for(int i = 0;i < height; i++) {
		delete []map[i];
	}
	delete []map;
	if (BGM) {
		delete BGM;
		BGM = nullptr;
	}
	if (shoeSound) {
		for (auto pair : *shoeSound) {
			if (pair.second) {//delete each sound effects corresponding each shoe in this map node
				delete pair.second;
				pair.second = nullptr;
			}
		}
		delete shoeSound;
		shoeSound = nullptr;
	}
}

Map* Map::MapSwitch(int x, int y, Character* player) {
	for (auto neighbor : neighbors) {
		if (x > neighbor.second->prevMap_exit_triger_Rect.left && x < neighbor.second->prevMap_exit_triger_Rect.right && y > neighbor.second->prevMap_exit_triger_Rect.top && y < neighbor.second->prevMap_exit_triger_Rect.bottom) {
			player->getCurrentMap()->getMap()[player->getPosition()->getRow()][player->getPosition()->getCol()] = '.'; //release last tile before change the map
			if (player->getMajor()->getName() == "Engineer") {//only main character who is controlled by user changes map, the bgm will be changed
				player->getCurrentMap()->stopBGM();
				neighbor.second->doorsound->Play();
			}
			player->setCurrentMap(neighbor.second->nextMap);
			if (player->getMajor()->getName() == "Engineer") {//only main character who is controlled by user changes map, the bgm will be changed
				player->getCurrentMap()->getBGM()->setVolume(volume);
				player->getCurrentMap()->playBGM();
			}
			player->getPosition()->moveCardinalDirectionPixel(neighbor.second->nextMap_enter_position.first, neighbor.second->nextMap_enter_position.second, neighbor.second->nextMap);
			player->getCurrentMap()->getMap()[player->getPosition()->getRow()][player->getPosition()->getCol()] = 'w'; //possese first tile
			player->getWalkEngine()->setOccupiedTile(player->getPosition()->getRow(), player->getPosition()->getCol());
			player->getWalkEngine()->SetWalkTarget(neighbor.second->nextMap_enter_position.first, neighbor.second->nextMap_enter_position.second);//clear old last map's targetXY
			player->getCamera()->setCamera(neighbor.second->nextMap,neighbor.second->nextMap_enter_position.first,neighbor.second->nextMap_enter_position.second);
			return neighbor.second->nextMap;
		}
	}
	return nullptr;
}
MapGraph::MapGraph(const vector<MAPSTRUCT> maps, const vector<pair<pair<string, door*>, pair<string, door*>>> doorpairs) {
	for (auto map : maps) {
		if (!AddMap(map)) {
			MessageBox(0, ("Map: " + map.NAME + " has already exists!").c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
		}
	}
	for (auto doorpair : doorpairs) {
		if (!PairDoors(doorpair.first.first, doorpair.first.second, doorpair.second.first, doorpair.second.second)) {
			MessageBox(0, ("Map: " + doorpair.first.first + " or " + doorpair.second.first + " has already exists!").c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
		}
	}
}
MapGraph::~MapGraph() {
	for (auto mapnode : MapCollections) {
		for (auto neighbor : mapnode.second->getNeighbor()) {
			delete neighbor.second;//delete each door*, door destructor will delete door sound, sound destructor will delete buffer, wfx, and psourcevoice
			neighbor.second = nullptr;
		}
		delete mapnode.second;//delete each map node, map destructor will delete map background music and 2D char map for collision, sound destructor will delete buffer, wfx and sourcevoice;
		mapnode.second = nullptr;
	}
}

bool MapGraph::AddMap(MAPSTRUCT map) {
	if (MapCollections.find(map.NAME) == MapCollections.cend()) {
		MapCollections[map.NAME] = new Map(map.ID, map.NAME, map.TILESIZE, map.STEP, map.WIDTH, map.HEIGHT, map.FILEPATH, map.pSoundDevice, map.BGMPATH, map.shoeSound, map.volume);
		return true;
	} else {
		return false;
	}
}


bool MapGraph::AddMap(int ID, string NAME, int TILESIZE, int STEP, int WIDTH, int HEIGHT, string filepath, SoundDevice* pSoundDevice, string bgm, unordered_map<string, Sound*>* shoeSoundpair, float& volume) {
	if (MapCollections.find(NAME) == MapCollections.cend()) {
		MapCollections[NAME] = new Map(ID, NAME, TILESIZE, STEP, WIDTH, HEIGHT, filepath, pSoundDevice, bgm, shoeSoundpair, volume);
		return true;
	} else {
		return false;
	}
}

bool MapGraph::PairDoors(string name1, door* door1, string name2, door* door2) {
	if (MapCollections.find(name1) == MapCollections.cend() || MapCollections.find(name2) == MapCollections.cend()) {
		return false;
	}
	Map* map1 = MapCollections[name1];
	Map* map2 = MapCollections[name2];
	door1->nextMap = map1;
	door2->nextMap = map2;
	map1->getNeighbor().insert({ map2, door2 });
	map2->getNeighbor().insert({ map1, door1 });

	return true;
}

Map* MapGraph::getMap(string name) {
	if (MapCollections.find(name) != MapCollections.cend()) {
		return MapCollections[name];
	} else {
		MessageBox(0, ("Map: " + name + " request does not exists!").c_str(), "Wrong", MB_ICONEXCLAMATION | MB_OK);
		return nullptr;
	}
}
bool MapGraph::MapPathDFSHelper(vector<Map*>& ret, unordered_set<Map*>& hash, Map* current, Map* target) {
	if (current == target) {
		return true;
	}
	for (auto neighbor : current->getNeighbor()) {
		if (hash.find(neighbor.second->nextMap) == hash.cend()) {
			hash.insert(current);
			ret.push_back(neighbor.second->nextMap);
			if (MapPathDFSHelper(ret, hash, neighbor.second->nextMap, target)) {
				return true;
			}
			ret.pop_back();
			hash.erase(neighbor.second->nextMap);
		}
	}
	return false;
}
//find the map-node path from current map node to the target map node
vector<Map*> MapGraph::findMapNodePath(Map* current, Map* target) {
	vector<Map*> ret;
	unordered_set<Map*> hash;
	MapPathDFSHelper(ret, hash, current, target);
	return ret;
}
