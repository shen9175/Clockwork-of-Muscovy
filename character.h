
#ifndef CHARACTER_H
#define CHARACTER_H

using namespace std;
class Core;
class Position
{
private:
	int x;//pixel
	int y;//pixel
	int row;
	int col;
	int direction;
public:
	Position() {}
	Position(int px, int py, int dir, Map* map) : x(px), y(py), row(py / map->getTileSize()), col(px / map->getTileSize()), direction(dir) {}
	int getX();//pixel
	int getY();//pixel
	int getRow();
	int getCol();
	bool equals();
	void setDir(int);
	int getDir();
	void moveCardinalDirectionPixel(int,int,Map*);//input is pixel
	void moveCardinalDirection(int,int,Map*);
};



class Compoent
{
private:
	string name;
	int value;
	int id;
public:
	string getName();
	int getValue();
	int getID();
};



class Item
{
private:
	
	int value;
	int id;
	float weight;
	int usage_type;//1: for attacking 2: for protection 3:equipment
	int usage_value;
	int usage_range;
	Compoent compoent;
	string name;
public:
    
	Item();
    Item(string name,int value, int id, float weight,int usage_type,int usage_value,int usage_range);
	string getName();
	int getValue();
	int getID();
	float getWeight();
	Compoent getCompoent();
	int getCompoentValue();
	int getUsageType();
	int getUsageValue();
	int getUsageRange();
	void setName(string);
};


class Inventory
{
private:
	Item* item[20]{nullptr};
	int nrOfitems;
	HWND  hwndItemListBox;  // Handle to Item list box
public:
	Inventory();
	~Inventory();
	int getNrOfitems();
	Item* getItem(Item* n);
	bool addItem(Core* p, Item* n);//need character stored core class instance pointer to get ItemList control handle
	bool removeItem(Core* p, Item* n); //need character stored core class instance pointer to get ItemList control handle
	float getTotalWeight();
	float getTotalValue();
	Item** getItemList();
};
class Skill
{
private:
	string name;
	string description;
	int usage_type;	//1 attacking,  2 restoring
	int amountOfDamage;
	int usage_range;
public:
	Skill();
	Skill(string name, string description, int usage_type, int amountOfDamage, int usage_range);
	string getName();
	string getDescription();
	bool doesDamage();
	int getDamage();
	int getUsageType();
	int getUsageRange();
};

class Major
{

private:
	Skill* skill[20]{nullptr};
	int nrOfskill;
	string name;

	int health;
	int dexterity;
	int endurance;
	int weapon_skill;
	int willpower;
	int base_physical_bonus;
	int base_skill_bonus;
	int physical_resistance;
	int skill_resistance;
	double physical_to_hit;
	double skill_to_hit;
	double defense;
	int level;
	int major_attribute;//low,medium,high

	bool ProvideCover;

public:
	Major();
	Major(Core* p, string name,int level);
	~Major();
	string getName();
	int getHealth();
	int getDexterity();
	int getWeaponSkill();
	int getWillPower();
	int getEndurance();
	int getBasePhysicalBonus();
	int getBaseSkillBonus();
	int getPhysicalResistance();
	int getSkillResistance();
	double getPhysicalToHit();
	double getSkillToHit();
	double getDefense();

	int low();
	int medium();
	int high();
	void updateHP();
	void updateBasePhysicalBonus();
	void updateBaseSkillBonus();
	void updatePhysicalResistance();
	void updateSkillResistance();
	void updatePhysicalToHit();
	void updateSkillToHit();
	void updateDefense();
	void setHP(int);

	void updateLevelup(Core* p, int lv, bool silent);//need character stored core class instance pointer to get noticequeue object

	bool canProvideCover();
	bool addSkill(Skill*);
	bool removeSkill(Skill);
	//int getArmorClass();
	Skill** getSkillList();
	Skill* getSkill(Skill*);
	int getNrOfSkill();
};

class Character
{
	friend class WalkEngine;
protected:
	string name;
	int id;
	int level;
	int currentHP;
	Core* pCore;	
	Position* position;//absolute pixel/tile in the whole map
	Map* currentMap;
	Major* major;
	Item* equipWeapon;
	Camera* camera;
	Inventory* inventory;
	string shoetype;//it's the key for retrieve corresponding footstep sound effect on specific map
	int AttackDamage;
	int MoveRange;
	int AttackRange;
	bool isSelected;
	bool isAttacking;
	bool isTakingCover;
	bool isMoving;
	bool alive;
	HBITMAP currentSprite;//walking engine will initial and update the currentSprites, no need set in the constructor
	//HBITMAP previousSprites;

public:
	Character(Core* p, Map* map, HBITMAP moving[4][3], HBITMAP stopping[4][1], int level, string majorname, int x, int y, int direction, bool live, string shoetype);
	~Character();
	Core* getpCore() { return pCore; }
	const string getName() {return name;}
	const int getID(){return id;}
	bool isAlive();
	void setAlive(bool);
	Map* getCurrentMap() { return currentMap; }
	Camera* getCamera() { return camera; }
	void setCurrentMap(Map* map) { currentMap = map; }
	Position* getPosition();
	Major* getMajor();
	Item* getEquipWeapon();
	Inventory* getInventory() { return inventory; }
	string& getShoeType() { return shoetype; }
	void setEquipWeapon(Item* weapon);
	HBITMAP getCurrentSprite();
	void setCurrentSprite(HBITMAP n);
	int getLevel();
	int getAttackDamage();
	int getAttackRange();
	int getMoveRange();
	bool select(Character*);
	bool attack(Character*);
	bool takeCover(Position);
	void moveTo(int x, int y, MapGraph* MapGraph, Map* targetMap);
	int getCurrentHP();
	void setCurrentHP(int n);
	//virtual Explore* getWalkEngine() = 0;
	virtual WalkEngine* getWalkEngine() = 0;
};


class Player : public Character
{
public:
	Player(Core* p, string majo, Map* map,HBITMAP moving[4][3], HBITMAP stopping[4][1], int level, int x, int y, int direction, bool live, double animationSpeed, string shoetype);
	~Player();
	string getSex();
	string getPersonality();
	string getBackstroy();
	Explore* getWalkEngine() { return walkengine; }
	int getAge();
	int getLevel();
	int getExperience();
	int getSP();
	void addExperience(int);
	bool interact(Character*);
private:
	int experience;
	int leveldata[20];
	int age;
	int SP;//skill points
	Explore* walkengine;
	string sex;
	string personality;
	string backstory;
};

class NPC : public Character
{
public:
	NPC(Core* p, string name, string majo, bool hostile, int frac, Map* map, int x, int y, int direction, int level, HBITMAP moving[4][3], HBITMAP stopping[4][1], bool live, HBITMAP currSprites, double animationSpeed, bool dialog_triger, string shoetype);
	~NPC();
	int getFraction();
	bool isHostile();
	bool getDialogtrigger() const { return dialogue_triger; }
	void setDialogtrigger(bool b) { dialogue_triger = b; }
	WalkEngine* getWalkEngine() { return walkengine; }
	int getTaskID() const { return taskID; }
	void clearTaskID() { taskID = 0; }
	void setTaskID(int id) { taskID = id; }
private:
	bool Hostile;
	int fraction;
	int taskID;
	bool dialogue_triger;
	WalkEngine* walkengine;
};


int rounded(double n);






#endif

