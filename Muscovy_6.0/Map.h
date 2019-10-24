#ifndef MAP_H
#define MAP_H

using namespace std;

class Map;
class Character;
struct door {
	pair<int, int> nextMap_enter_position;
	RECT prevMap_exit_triger_Rect;
	Map* nextMap;
	Sound* doorsound;
	door (int x, int y, int left, int top, int right, int bottom, IXAudio2* pxaudio, string filename/*Sound* sound*/) : nextMap_enter_position(x, y), prevMap_exit_triger_Rect{ left, top, right, bottom }, doorsound(new Sound(pxaudio, filename, 0)/*sound*/) {}
	door() { nextMap_enter_position = { 0,0 }, prevMap_exit_triger_Rect = { 0,0,0,0 }; nextMap = nullptr; doorsound = nullptr; } // unordered_map need default constructor for door
	~door(){ delete doorsound; doorsound = nullptr;}
};
struct MAPSTRUCT {
	int ID;
	string NAME;
	int TILESIZE;
	int STEP;
	int WIDTH;
	int HEIGHT;
	string FILEPATH;
	IXAudio2* pXaudio2;
	string BGMPATH;
	unordered_map<string, Sound*>* shoeSound;
	float& volume;
	MAPSTRUCT(int id, string name, int tilesize, int step, int width, int height, string filepath, IXAudio2* pxaudio, string bgm, unordered_map<string, Sound*>* ShoeSoundpair, float& gvolume) : ID(id), NAME(name), TILESIZE(tilesize), STEP(step), WIDTH(width), HEIGHT(height), FILEPATH(filepath), pXaudio2(pxaudio), BGMPATH(bgm), shoeSound(ShoeSoundpair), volume(gvolume) {}
};
class Map {
private:
	string name;
	int mapID;
	int tile_size;
	int step;
	int width;
	int height;
	string mapfile_path;
	char** map;
	Sound* BGM;
	float& volume;
	unordered_multimap<Map*, door*> neighbors;
	unordered_map<string, Sound*>* shoeSound;//ideally should each each ground material has a shoe-sound pair hashmap for each map and shoe but here we assume each map has the same ground material for simplicity.
	//each player carries it's shoe parameter and call here to retrieve corresponding footstep sound effects.
public:
	Map(int ID, string NAME, int TILESIZE, int STEP, int WIDTH, int HEIGHT, string filepath, IXAudio2* pXAuodio2, string bgm, unordered_map<string, Sound*>* shoeSoundpair, float& gVolume);
	~Map();
	Map* MapSwitch(int x, int y, Character* player);
	void setTileSize(int size) { tile_size = size; }
	void setWidth(int w) { width = w; }
	void setHeight(int h) { height = h; }
	void setMap(char** m) { map = m; }
	void setMapFile(string path) { mapfile_path = path; }
	void playBGM() { BGM->Play(); }
	void stopBGM() { BGM->Stop(); }
	string getMapName() const { return name; }
	int getMapID() const { return mapID; }
	int getTileSize() const { return tile_size; }
	int getMovingSteps() const{ return step; }
	int getTileWidth() const { return width; }
	int getTileHeight() const { return height; }
	int getPixelWidth() const { return tile_size * width; }
	int getPixelHeight() const { return tile_size * height; }
	Sound* getBGM() const { return BGM; }
	char** getMap() { return map; }
	unordered_multimap<Map*, door*>& getNeighbor() { return neighbors; }
	Sound* getShoeSound(string shoe) const { return shoeSound->at(shoe); }
	

};



class MapGraph {
private:
	unordered_map<string, Map*> MapCollections;
	bool MapPathDFSHelper(vector<Map*>& ret, unordered_set<Map*>& hash, Map* current, Map* target);
public:
	MapGraph() {}
	MapGraph(const vector<MAPSTRUCT> maps, const vector<pair<pair<string, door*>, pair<string, door*>>> doorpairs);
	~MapGraph();
	bool AddMap(MAPSTRUCT map);
	bool AddMap(int ID, string NAME, int TILESIZE, int STEP, int WIDTH, int HEIGHT, string filepath,IXAudio2* pXaudio2, string bgm, unordered_map<string, Sound*>* shoeSoundpair, float& volume);
	bool PairDoors(string name1, door* door1, string name2, door* door2);
	Map* getMap(string name);
	vector<Map*> findMapNodePath(Map* current, Map* target);
};

#endif
