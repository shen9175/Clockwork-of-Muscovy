#include<string>
#include<Windows.h>
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
#include <queue>
#include <vector>
#include <ctime>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "Sound.h"
#include "pqastar.h"
#include "timer.h"
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


#include <iostream>
#include <string>
#include <fstream>

Character::~Character() {
	delete position;
	delete inventory;
	delete major;
	delete equipWeapon;
	delete camera;
}



Position* Character::getPosition()
{
	return position;
}

int Character::getAttackRange()
{
	return AttackRange;
}
/*
Position* Character::getBattlePosition()
{
	return battleposition;
}
void Character::setBattlePosition(int x, int y)
{
	battleposition.moveCardinalDirection(x,y);
}*/
Major* Character::getMajor()
{
	return major;
}
/*Major::Major()
{
    nrOfskill=0;

}*/
string Major::getName()
{
	return name;
}
bool Major::canProvideCover()
{
	return ProvideCover;
}
bool Major::addSkill(Skill* n)
{
	skill[nrOfskill]=n;
	nrOfskill++;
	return true;
}
bool Major::removeSkill(Skill n)
{
	return true;
}

Skill** Major::getSkillList()
{
    return skill;
}
Skill* Major::getSkill(Skill* n)
{
	int i;
	for(i=0;i<nrOfskill;i++)
	{
		if(n->getName()==skill[i]->getName())
			break;
	}
	return skill[i];
}
int Major::getNrOfSkill()
{
    return nrOfskill;
}
Major::Major()
{
}
Major::Major(Core* p, string n,int lv)
{
	name=n;
	level=lv;
	nrOfskill=0;
	updateLevelup(p, level,true);
	Skill* s1=new Skill("Fire","one kind of element spell",1,20,5);
    Skill* s2=new Skill("Cure","one kind of healing spell",2,25,5);
    Skill* s3=new Skill("Thunder","one kind of element spell",1,20,5);
    addSkill(s1);
    addSkill(s2);
    addSkill(s3);	
}

Major::~Major()
{
	for(int i=0;i<20;i++)
	{
		if(skill[i]!=nullptr)
		{
			delete skill[i];
			skill[i]=nullptr;
		}
	}
}
int Major::getHealth()
{
	return health;
}
int Major::getEndurance()
{
	return endurance;
}
int Major::getDexterity()
{
	return dexterity;
}
int Major::getWeaponSkill()
{
	return weapon_skill;
}
int Major::getWillPower()
{
	return willpower;
}

int Major::getBasePhysicalBonus()
{
	return base_physical_bonus;
}
int Major::getBaseSkillBonus()
{
	return base_skill_bonus;
}
int Major::getPhysicalResistance()
{
	return physical_resistance;
}
int Major::getSkillResistance()
{
	return skill_resistance;
}
double Major::getPhysicalToHit()
{
	return physical_to_hit;
}
double Major::getSkillToHit()
{
	return skill_to_hit;
}
double Major::getDefense()
{
	return defense;
}
int Major::low()
{	
	int r=0; 
		for(int i=1;i<=level;i++)
		{
			if(i==1)
				r=3;
			else
				r+=(int)(i*0.5);
		}
		if(r>13)
			r=13;
		return r;
}
int Major::medium()
{
		int r=0; 
		for(int i=1;i<=level;i++)
		{
			if(i==1)
				r=5;
			else
				r+=i;
		}
		if(r>25)
			r=25;
		return r;
}
int Major::high()
{
		int r=0; 
		for(int i=1;i<=level;i++)
		{
			if(i==1)
				r=7;
			else
				r+=(int)(i*1.5);
		}
		if(r>37)
			r=37;
		return r;
}
void Major::updateHP()
{

	if(level==1)
	{
	switch(major_attribute)
	{
		case 1:	//low
			health=endurance*2+1;
			break;
		case 2: //medim
			health=endurance*2+2;
			break;
		case 3: //high
			health=endurance*2+3;
			break;
		default:
			break;
		}
	}
	else
		health+=endurance*2;
}

void Major::updateBasePhysicalBonus()
{
	if(level==1)
		base_physical_bonus=rounded(weapon_skill*0.25)+(major_attribute-1);
	else
		base_physical_bonus+=rounded(weapon_skill*0.25);
}
void Major::updateBaseSkillBonus()
{
	if(level==1)
		base_skill_bonus=rounded(willpower*0.25)+(major_attribute-1);
	else
		base_skill_bonus+=rounded(willpower*0.25);
}
void Major::updatePhysicalResistance()
{
	if(level==1)
		physical_resistance=rounded(endurance*0.2)+major_attribute;
	else
		physical_resistance+=rounded(endurance*0.2);
}
void Major::updateSkillResistance()
{
	if(level==1)
		skill_resistance=rounded(willpower*0.2)+major_attribute;
	else
		skill_resistance+=rounded(willpower*0.2);
}

void Major::updatePhysicalToHit()
{
	if(level==1)
	{
		switch(major_attribute)
		{
		case 1://low
			physical_to_hit=0.45;
			break;
		case 2://medium
			physical_to_hit=0.50;
			break;
		case 3://high
			physical_to_hit=0.55;
			break;
		default:
			break;
		}
	}
	else
		physical_to_hit+=dexterity*0.01;
}
void Major::updateSkillToHit()
{
		if(level==1)
	{
		switch(major_attribute)
		{
		case 1://low
			skill_to_hit=0.45;
			break;
		case 2://medium
			skill_to_hit=0.50;
			break;
		case 3://high
			skill_to_hit=0.55;
			break;
		default:
			break;
		}
	}
		else
			skill_to_hit+=willpower*0.01;
}
void Major::updateDefense()
{
	if(level==1)
	{
		switch(major_attribute)
		{
		case 1://low
			defense=0.20;
			break;
		case 2://medium
			defense=0.25;
			break;
		case 3://high
			defense=0.30;
			break;
		default:
			break;
		}
	}
	else
		defense+=dexterity*0.005;
}

void Major::updateLevelup(Core* p, int lv,bool silent)
{
	this->level = lv;
	if (!silent) {
		p->noticequeue->push("Your level has upgraded to " + to_string(static_cast<long long>(lv)) + " !");
	}
	if(name=="Engineer")
	{
		weapon_skill=low();
		dexterity=low();
		endurance=high();
		willpower=high();
		major_attribute=1;
	}
	else if(name=="Guardsman")
	{
		weapon_skill=high();
		dexterity=low();
		endurance=high();
		willpower=low();
		major_attribute=2;
	}
	else if(name=="Chemist")
	{
		weapon_skill=medium();
		dexterity=medium();
		endurance=low();
		willpower=high();
		major_attribute=3;
	}
	else if(name=="Philosopher")
	{
		weapon_skill=low();
		dexterity=medium();
		endurance=medium();
		willpower=high();
		major_attribute=1;
	}
	else ;
	updateHP();
	updateBasePhysicalBonus();
	updateBaseSkillBonus();
	updatePhysicalResistance();
	updateSkillResistance();
	updatePhysicalToHit();
	updateSkillToHit();
	updateDefense();

}

void Major::setHP(int hp)
{
	health=hp;
}
int Character::getCurrentHP()
{
	return currentHP;
}
void Character::setCurrentHP(int n)
{
	currentHP=n;
}
HBITMAP Character::getCurrentSprite()
{
	return currentSprite;
}
void Character::setCurrentSprite(HBITMAP n)
{
	currentSprite=n;
}


int Character::getLevel()
{
	return level;
}

int Character::getAttackDamage()
{
	return AttackDamage;
}
int Character::getMoveRange()
{
	return MoveRange;
}


Item* Character::getEquipWeapon()
{
	return equipWeapon;
}
void Character::setEquipWeapon(Item* weapon)
{
	equipWeapon=weapon;
}

bool Character::select(Character* n)
{
		return true;
}
bool Character::attack(Character* n)
{
	return true;
}
bool Character::takeCover(Position n)
{
	return true;
}

void Character::moveTo(int x, int y, MapGraph* MapGraph, Map* targetMap)
{
	if (currentMap == targetMap) {//if it is in the same map, call Astar to move to it
		getWalkEngine()->SetWalkTarget(x, y);
		return;
	}
	vector<Map*> path = MapGraph->findMapNodePath(currentMap, targetMap);//if it is not in the same map, use DFS find the map node path
	if (path.empty()) {
		pCore->noticequeue->push("There is no way to" + targetMap->getMapName() + " !");
		return;
	}
	Map* current = currentMap;//put each map to the next map's exit trigger center point to the moving queue let the walkengine walk through each map until the last one
	for (auto map : path) {
		//split the steps for debug;
		int tx, ty;
		int distance = INT_MAX;
		int left, right, top, bottom;
		auto iter = current->getNeighbor().lower_bound(map);
		auto end = current->getNeighbor().upper_bound(map);
		//if there are multi doors to the next map node (like jeb1 to jeb2 there are 2 stairs to jeb2: bottom and left)
		//we simpliy go to the shortest linear distance door and don't consider the real walking distance for simplicity.
		while (iter != end) {
			int tempx, tempy;
			left = iter->second->prevMap_exit_triger_Rect.left;
			right = iter->second->prevMap_exit_triger_Rect.right;
			bottom = iter->second->prevMap_exit_triger_Rect.bottom;
			top = iter->second->prevMap_exit_triger_Rect.top;
			tempx = left + (right - left) / 2;
			tempy = top + (bottom - top) / 2;
			int temp = ((tempx - position->getX()) * (tempx - position->getX()) + (tempy - position->getY()) * (tempy - position->getY()));
			if (temp < distance) {
				distance = temp;
				tx = tempx;
				ty = tempy;
			}
			++iter;
		}
			getWalkEngine()->getMovingQueue().push({ make_pair(tx,ty),current->getMapID() });
			current = map;
		}
	getWalkEngine()->getMovingQueue().push({ make_pair(x, y), targetMap->getMapID() });//in the last map, call Astart to move to the target point
}
bool Character::isAlive()
{
	return alive;
}
void Character::setAlive(bool a)
{
	alive=a;
}


Skill::Skill()
{
}
Skill::Skill(string n, string d, int ut, int a, int ur)
{
    name=n;
	description=d;
	usage_type=ut;	//1 attacking,  2 restoring
	amountOfDamage=a;
	usage_range=ur;
}
string Skill::getName()
{
	return name;
}
bool Skill::doesDamage()
{
	return true;
}
string Skill::getDescription()
{
	return description;
}
int Skill::getDamage()
{
	return amountOfDamage;
}
int Skill::getUsageType()
{
	return usage_type;
}
int Skill::getUsageRange()
{
	return usage_range;
}


Character::Character(Core* p, Map* map,HBITMAP moving[4][3], HBITMAP stopping[4][1], int level, string majorname, int x, int y, int direction, bool live, string shoe)
	: pCore(p), currentMap(map), alive(live), level(level),major(new Major(p, majorname, level)), inventory(new Inventory),
	position(new Position(x,y,direction, map)), camera(new Camera(map->getPixelWidth(), map->getPixelHeight(), position->getX(), position->getY())), shoetype(shoe){

	currentHP=this->getMajor()->getHealth();
	Sound* currentSound = this->currentMap->getShoeSound(shoe);
	Character* CameraPlayer = p->sprites.empty() ? this : p->sprites[0];
	currentSound->initialListenerEmitter(CameraPlayer->getCamera()->getLeft() + (CameraPlayer->getCamera()->getRight() - CameraPlayer->getCamera()->getLeft()) / 2.0f,
										 CameraPlayer->getCamera()->getTop() + (CameraPlayer->getCamera()->getBottom() - CameraPlayer->getCamera()->getTop()) / 2.0f,
										 200.0f,
										 static_cast<float>(x),
										 static_cast<float>(y),
										 0.0f
										);
}

NPC::NPC(Core* p, string name, string majo, bool hostile,int frac, Map* map,int x, int y, int direction, int level, HBITMAP moving[4][3], HBITMAP stopping[4][1],bool live, HBITMAP currSprites, double animationSpeed,bool dialog_triger, string shoetype) : Character(p, map, moving, stopping, level, majo, x, y, direction, live, shoetype)
{
	walkengine = new WalkEngine(this, this->currentMap, moving, stopping, animationSpeed, this->shoetype);//use Character's shoetype, not parameter's shoetype, that one will destroy after this call
	currentSprite = currSprites == nullptr ? stopping[rand() % 4][0] : currSprites;
	fraction=frac;
	Hostile=hostile;
	this->name = name;
	alive=live;
	dialogue_triger = true;
	taskID = 0;
}
NPC::~NPC()
{
	delete walkengine;
}
int NPC::getFraction()
{
	return fraction;
}

bool NPC::isHostile()
{
	return Hostile;
}

Player::Player(Core* p, string majo, Map* map,HBITMAP moving[4][3], HBITMAP stopping[4][1], int level, int x, int y, int direction, bool live, double animationSpeed, string shoetype) : Character(p, map, moving, stopping, level, majo, x, y, direction, live, shoetype)
{
	walkengine = new Explore(pCore, this, this->currentMap, moving, stopping, animationSpeed, this->shoetype);//use Character's shoetype, not parameter's shoetype, that one will destroy after this call
	experience=0;
	SP=2;
	AttackRange=5;
	experience=0;
	leveldata[0]=0;
	leveldata[1]=10;
	leveldata[2]=25;
	leveldata[3]=50;
	leveldata[4]=75;
	leveldata[5]=100;
	leveldata[6]=150;
	leveldata[7]=200;
	leveldata[8]=250;
	leveldata[9]=350;
	leveldata[10]=450;
	leveldata[11]=600;
	leveldata[12]=750;
	leveldata[13]=900;
	leveldata[14]=1100;
	leveldata[15]=1300;
	leveldata[16]=1500;
	leveldata[17]=1750;
	leveldata[18]=2000;
	leveldata[19]=2500;


	if(majo=="Engineer")
	{
		name="Reilly R. Lillywhite";
		sex="Female";
		age=18;
		personality="Soft spoken and a little withdraw, she has a little trouble fitting in. What she lacks in charisma she makes up for in intelligence. She abhors most physical labor, instead spending her time in a laboratory.";
		backstory="The youngest of four brothers, Reilly has always been under the watchful eye of some authority figure. Falling in love with the mechanical arts she is the only one in her family to stray from the traditional mining profession. With a set of great inventions on her resume she was graciously accepted in to the University of Muscovy. Additionally, she works for the engineering labs in her off time and has gained access to a number of different crafting catalogs.";
		currentSprite=pCore->player_female_stopping[2][0];
	}
	else if(majo=="Guardsman")
	{
		name="Harlowe Bradford";
		sex="Male";
		age=18;
		personality="Driven, dedicated, and good natured Harlowe believes in fighting for the betterment of all. Highly charismatic, he enjoys placing himself in various leadership rolls. Despite all of these positive qualities it is rumored that he is somewhat bloodthirsty.";
		backstory="His parents having passed away at a young age, Harlowe was raised by his grandmother. Focusing his efforts in sports and extracurricular activities he strengthened himself in to a prime athlete. He joined the military as soon as possible and was fast tracked in to a leadership position. This landed him a spot at the University of Muscovy in the Military Leadership department.";
		currentSprite=pCore->player_male_stopping[2][0];
	}
	else if(majo=="Chemist")
	{
		name="Elizabeth Crowe";
		sex="Female";
		age=16;
		personality="Considered the school's resident mad scientist, she is seen as being obsessed with her work. High energy, she operates her life at a fast pace. Despite her strange demeanor she is very friendly.";
		backstory="The progeny of two of Muscovy's star chemists, Elizabeth was destined for great things since an early age. She excelled at her studies, quickly leaving those in her age group behind. This has had a profound influence on her social skills, and as such she has gone through most of her life with little to no friends. She was a lab assistant on the team that developed the new chemical product known as goo. At the age of 16 she was admitted to the University of Muscovy and is following closely in her parent's footsteps.";
		currentSprite=pCore->player_female_stopping[2][0];
	}
	else if(majo=="Philosopher")
	{
		name="Mannfred the Ponderer";
		sex="Male";
		age = 40;
		personality="A mostly quiet individual, he is known for speaking his mind when the time warrants it. He carries a large weight with him, and often seems sullen. Free spirited he spends most his time outside doing what he pleases.";
		backstory="Mannfred has been something of an enigma as far back as anyone can remember. An outcast, he spent most of his time in Muscovy tending to the plant and animal life. During a visit from the wandering philosophers who explore the countryside Mannfred made a decision to join them on their journey. While traveling he learned of the world and it's pain, constantly drawn in to a contemplative state. Shortly before joining the University of Muscovy to officiate his place as a Natural Philosopher he shed his last name and adopted the moniker the Ponderer.";
		currentSprite=pCore->player_male_stopping[2][0];
	}
	else ;
	

}
Player::~Player()
{
	delete walkengine;
}

int Player::getExperience()
{
	return experience;
}
void Player::addExperience(int n)
{
	pCore->noticequeue->push("You got " + to_string(static_cast<long long>(n)) + " points experience !");
	experience += n;
	if(experience>2500)
	{
		experience=2500;
		level=20;
	}
	else
	{
		if(experience > leveldata[level])
		{
			this->getMajor()->updateLevelup(this->pCore, ++level,false);
			this->currentHP = this->getMajor()->getHealth();//since total health upgrade, the current HP should be full.
		}
	}
	
}
bool Player::interact(Character* n)
{
	return true;
}
string Player::getSex()
{
	return sex;
}
string Player::getPersonality()
{
	return personality;
}
string Player::getBackstroy()
{
	return backstory;
}
int Player::getAge()
{
	return age;
}
int Player::getLevel()
{
	return level;
}
int Player::getSP()
{
	return SP;
}
Inventory::Inventory()
{
    nrOfitems=0;
}
Inventory::~Inventory()
{
	for(int i=0;i<nrOfitems;i++)
	{
		SendMessage(hwndItemListBox,CB_DELETESTRING,(WPARAM)i, 0);
		delete item[i];
		item[i]=nullptr;
	}
}
int Inventory::getNrOfitems()
{
	return nrOfitems;
}
Item* Inventory::getItem(Item* n)
{
	int i;
	for(i=0;i<nrOfitems;i++)
	{
		if(n->getID()==item[i]->getID())
			break;
	}
	return item[i];
}

bool Inventory::addItem(Core* p, Item* n)
{
	if (hwndItemListBox == nullptr) {
		hwndItemListBox = p->hwndItemListBox;
	}
	item[nrOfitems]=n;
	SendMessage(p->hwndItemListBox,CB_INSERTSTRING,(WPARAM)nrOfitems,(LPARAM)n->getName().c_str());
	p->noticequeue->push("You got the" + n->getName() + "!");
	nrOfitems++;
	return true;
}
bool Inventory::removeItem(Core* p,Item* n)
{
	int position=0;
	for(int i=0;i<nrOfitems;i++)
	{
		if(n->getID()==item[i]->getID())
		{
			position=i;
			break;
		}
	}
	if(position==nrOfitems)// 
		return false;
	else
	{
		delete item[position];
		for(int i=position;i<nrOfitems-1;i++)
		{
			SendMessage(p->hwndItemListBox, CB_DELETESTRING, (WPARAM)i,0);
			SendMessage(p->hwndItemListBox, CB_INSERTSTRING, (WPARAM)i, (LPARAM)item[i + 1]->getName().c_str());
			item[i]=item[i+1];
		}
		SendMessage(p->hwndItemListBox, CB_DELETESTRING, (WPARAM)(nrOfitems - 1),0);
		//for(int i=nrOfitems;i<20;i++)
		//    item[i]->setName("");
		nrOfitems--;
		
	}
	return true;
}
float Inventory::getTotalValue()
{
	float r=0;
	return r;
}
float Inventory::getTotalWeight()
{
	float r=0;
	return r;
}

Item** Inventory::getItemList()
{
    return item;
}

Item::Item()
{
}
Item::Item(string n,int v, int ID, float w,int ut,int uv,int ur)
{
	name=n;
	value=v;
	id=ID;
	weight=w;
	usage_type=ut;//1: for attacking 2: for protection
	usage_value=uv;
	usage_range=ur;
}
string Item::getName()
{
	return name;
}
void Item::setName(string n)
{
	name=n;
}
int Item::getValue()
{
	return value;
}
int Item::getID()
{
	return id;
}
float Item::getWeight()
{
	return weight;
}

Compoent Item::getCompoent()
{
	return compoent;
}

int Item::getCompoentValue()
{
	int r=0;
	return r;
}
int Item::getUsageType()
{
	return usage_type;
}
int Item::getUsageValue()
{
	return usage_value;
}
int Item::getUsageRange()
{
	return usage_range;
}
string Compoent::getName()
{
	return name;
}
int Compoent::getID()
{
	return id;
}
int Compoent::getValue()
{
	return value;
}

int Position::getX()
{
	return x;
}
int Position::getY()
{
	return y;
}
bool Position::equals()
{
	//?
	return true;
}

int Position::getCol()
{
	return col;
}
int Position::getRow()
{
	return row;
}

void Position::moveCardinalDirection(int n,int m, Map* map)
{
	row = n;
	col = m;
	y = row * map->getTileSize();
	x = col * map->getTileSize();
}
void Position::moveCardinalDirectionPixel(int xp, int yp, Map* map)
{
	x =xp;
	y = yp;

	col = xp / map->getTileSize();
	row = yp / map->getTileSize();
}
void Position::setDir(int d)
{
	direction=d;
}
int Position::getDir()
{
	return direction;
}

int rounded(double n)
{
	if(n-(int)n>=0.5)
		return (int)(n+1);
	else
		return (int)n;
}
