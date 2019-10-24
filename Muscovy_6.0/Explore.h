#ifndef EXPLORE_H
#define EXPLORE_H
class Core;
class Battle;
extern fstream outfile;




class Core;
class Explore : public WalkEngine{
	friend class Core;
public:
	Explore(Core* p, Character* user, Map* &inmap, HBITMAP move[][3], HBITMAP stop[][1], double animationSpeed, string& shoetype);
	~Explore();
	void Control() override;
	void Update() override;
	int isAround(int d_pixel, int a, int b, int currentTask);
	void antiAroundfunc(int d_pixel, int i);
	//pair<bool, int> getAntiRound() { return antiAround; }
	//void setAntiAround(bool b, int index) { antiAround.first = b; antiAround.second = index; }

private:
	Core* pCore;
	bool hold;
	int cursorX, cursorY;//current mouse left click position
	
};

#endif