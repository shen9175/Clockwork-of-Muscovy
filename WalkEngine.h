#ifndef WALKENGINE_H
#define WALKENGINE_H
class Character;
struct TARGET {
	pair<int, int> position;
	int map;
};
class WalkEngine {
public:
	WalkEngine(Character* user, Map* &inmap, HBITMAP moving[][3], HBITMAP stopping[][1], double aniSpeed, string& shoetype);
	virtual void Control();
	virtual void Update();
	virtual void SetWalkTarget(int x, int y) { TargetX = x; TargetY = y; }
	virtual queue<TARGET>& getMovingQueue() { return movingqueue; }
	int getTargetX() const { return TargetX; }
	int getTargetY() const { return TargetY; }
	pair<int, int> getOccupiedTile() { return occupiedTile; }
	void setOccupiedTile(int row, int col) { occupiedTile = { row, col }; }
	void resetWalkEngine();
	
protected:
	Move* pmove;
	Map* &map;//the map pointer in the character
	Character* EngineUser;
	int TargetX, TargetY;
	int preTargetX, preTargetY;
	Timer animationTimer;
	Timer footstepSoundTimer;
	Timer footSpeedTimer;
	queue<TARGET> movingqueue;
	HBITMAP (*MovingSprites)[3];//pointer to an array of 3 HBITMAP type element. read it from inside out:  inside, a pointer, which points an array size of 3 which contains HBITMAP
	HBITMAP (*StoppingSprites)[1];//HBITMAP *p[3] is an array size of 3 which contains HBITMAP* type elemnt
	pair<int, int> occupiedTile;
	double animationSpeed;
	int animationPic;
	string& shoetype;
};
#endif
