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






//====battle.cpp dedicated global variable definition===========//
HRGN bgRgn;
HBRUSH hBrush;
RECT textRect; 

string int_string(int n)
{
	string r;
	char buffer[1024];
	sprintf_s(buffer,"%d",n);
	r=buffer;
	return r;
}
bool Battlehelper::Checkalive(Character* c)
{
	bool alive;
	if(c->getCurrentHP()>0)
		alive=true;
	else
		alive=false;
	return alive;
}
string Battlehelper::DealDamage(Character* attacker, Character* defender)
{
	string r;
	double attackSuccess=attacker->getMajor()->getPhysicalToHit() - defender->getMajor()->getDefense();
	if(attackSuccess < 0)
		attackSuccess = 0;
	int totalDamage = attacker->getMajor()->getBasePhysicalBonus() + attacker->getEquipWeapon()->getUsageValue()
						- defender->getMajor()->getPhysicalResistance();
	if(totalDamage<0)
		totalDamage=0;
	if(rand()/(double)RAND_MAX < attackSuccess)
	{
		defender->setCurrentHP((defender->getCurrentHP()-totalDamage > 0) ? (defender->getCurrentHP() - totalDamage) : 0);
		r = attacker->getName() + " attacks " + defender->getName() + " "+ int_string(totalDamage) + "points HP.";
	}
	else
		r="Attacking miss!";
	if(defender->getCurrentHP()<=0)
		defender->setAlive(false);

	return r;
}


string Battlehelper::DealItem(Item* item, Character* attacker, Character* defender)
{
	string r;

	/*
	if(item==NULL)
		r="Nothing is chosen.";*/
	if(item->getUsageType()==1)//attacking type
	{
		defender->setCurrentHP(defender->getCurrentHP()-item->getUsageValue());
		attacker->getInventory()->removeItem(pCore,item);
		r="Item: "+item->getName()+"damages "+defender->getName()+int_string(item->getUsageValue())+" points HP.";
	}
	else if(item->getUsageType()==2)//restoring health type
	{
		if(attacker->getCurrentHP()+item->getUsageValue()>attacker->getMajor()->getHealth())
		    attacker->setCurrentHP(attacker->getMajor()->getHealth());
		else
		attacker->setCurrentHP(attacker->getCurrentHP()+item->getUsageValue());
		attacker->getInventory()->removeItem(pCore, item);
		r=attacker->getName()+" uses Item: "+item->getName()+" restoring "+int_string(item->getUsageValue())+" points HP.";
	}
	else
		r="The item can not be used here!";

	return r;
}

string Battlehelper::DealSkill(Skill* skill, Character* attacker, Character* defender)
{
	string r;
	/*if(skill==NULL)
		r="Nothing is chosen.";*/
	if(skill->getUsageType()==1)//attacking type
	{
		defender->setCurrentHP(defender->getCurrentHP()-skill->getDamage()>0?defender->getCurrentHP()-skill->getDamage():0);
		r=attacker->getName()+" uses skill "+skill->getName()+" damaging "+defender->getName()+" by "+int_string(skill->getDamage())+" points HP.";
	}
	else if(skill->getUsageType()==2)//heal type
	{
		if(attacker->getCurrentHP()+skill->getDamage()>attacker->getMajor()->getHealth())
		    attacker->setCurrentHP(attacker->getMajor()->getHealth());
		else
		attacker->setCurrentHP(attacker->getCurrentHP()+skill->getDamage());
		r=attacker->getName()+" uses skill "+skill->getName()+" restoring "+attacker->getName()+" by "+int_string(skill->getDamage())+" points HP.";
	}
	else
	{
		//under construction
	}
	return r;
}


vector<Position> Battlehelper::availableMove(Character* c)
{
	vector<Position> r;
	int x=c->getPosition()->getRow();
	int y=c->getPosition()->getCol();
	for(int i=0;i<c->getMoveRange();i++)
	{
		for(int j=1;j<=c->getMoveRange()-i;j++)
		{
			if(x+i<20&&y+j<20)//map edge verification
			{
				if(map[x+i][y+j]!='w'&&map[x+i][y+j]!='#'&&map[x+i][y+j]!='o')
				{
					bool occupied=false;
					for(int k=0;k < static_cast<int>(pCore->sprites.size()); k++)
					{
						if(pCore->sprites[k]->getPosition()->getRow()==x+i&&pCore->sprites[k]->getPosition()->getCol()==y+j)
						{
							occupied=true;
							break;
						}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x+i,y+j, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

			if(x+j<20&&y-i>0)//map edge verification
			{
				if(map[x+j][y-i]!='w'&&map[x+j][y-i]!='#'&&map[x+j][y-i]!='o')
				{
					bool occupied=false;
					for(int k = 0;k < static_cast<int>(pCore->sprites.size()); k++)
					{
						if(pCore->sprites[k]->getPosition()->getRow()==x+j&&pCore->sprites[k]->getPosition()->getCol()==y-i)
						{
							occupied=true;
							break;
						}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x+j,y-i, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

			if(x-i>0&&y-j>0)//map edge verification
			{
				if(map[x-i][y-j]!='w'&&map[x-i][y-j]!='#'&&map[x-i][y-j]!='o')
				{
					bool occupied=false;
					for(int k = 0; k < static_cast<int>(pCore->sprites.size()); k++)
					{
						if(pCore->sprites[k]->getPosition()->getRow()==x-i&&pCore->sprites[k]->getPosition()->getCol()==y-j)
						{
							occupied=true;
							break;
						}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x-i,y-j, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

			if(x+j<20&&y-i>0)//map edge verification
			{
				if(map[x+j][y-i]!='w'&&map[x+j][y-i]!='#'&&map[x+j][y-i]!='o')
				{
					bool occupied=false;
					for(int k = 0; k < static_cast<int>(pCore->sprites.size()); k++)
					{
						if(pCore->sprites[k]->getPosition()->getRow()==x+j&&pCore->sprites[k]->getPosition()->getCol()==y-i)
						{
							occupied=true;
							break;
						}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x+j,y-i, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

		}
	}

	return r;
}

vector<Position> Battlehelper::availableAttack(Character* c)
{


	vector<Position> r;
	int x=c->getPosition()->getRow();
	int y=c->getPosition()->getCol();
	for(int i=0;i<c->getEquipWeapon()->getUsageRange();i++)
	{
		for(int j=1;j<=c->getEquipWeapon()->getUsageRange()-i;j++)
		{
			if(x+i<20&&y+j<20)//map edge verification
			{
				if(map[x+i][y+j]!='w'&&map[x+i][y+j]!='#'&&map[x+i][y+j]!='o')
				{
					bool occupied=false;
					for(int k = 0; k < static_cast<int>(pCore->sprites.size()); k++)
					{
						if(pCore->sprites[k]->getPosition()->getRow()==x+i&&pCore->sprites[k]->getPosition()->getCol()==y+j)
						{
							if(!static_cast<NPC*>(pCore->sprites[k])->isHostile())
							{
								occupied=true;
								break;
							}
						}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x+i,y+j, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

			if(x+j<20&&y-i>0)//map edge verification
			{
				if(map[x+j][y-i]!='w'&&map[x+j][y-i]!='#'&&map[x+j][y-i]!='o')
				{
					bool occupied=false;
					for(int k = 0; k < static_cast<int>(pCore->sprites.size()); k++)
					{
							if(!static_cast<NPC*>(pCore->sprites[k]))
							{
								occupied=true;
								break;
							}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x+j,y-i, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

			if(x-i>0&&y-j>0)//map edge verification
			{
				if(map[x-i][y-j]!='w'&&map[x-i][y-j]!='#'&&map[x-i][y-j]!='o')
				{
					bool occupied=false;
					for(int k = 0;k < static_cast<int>(pCore->sprites.size()); k++)
					{
							if(!static_cast<NPC*>(pCore->sprites[k]))
							{
								occupied=true;
								break;
							}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x-i,y-j, c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

			if(x+j<20&&y-i>0)//map edge verification
			{
				if(map[x+j][y-i]!='w'&&map[x+j][y-i]!='#'&&map[x+j][y-i]!='o')
				{
					bool occupied=false;
					for(int k = 0; k < static_cast<int>(pCore->sprites.size()); k++)
					{
							if(!static_cast<NPC*>(pCore->sprites[k]))
							{
								occupied=true;
								break;
							}
					}

					if(!occupied)
					{
						Position p;
						p.moveCardinalDirection(x+j,y-i,c->getCurrentMap());
						r.push_back(p);
					}
				}
			}

		}
	}

	return r;

}
Character* Battlehelper::CheckValidAttack(Character* attacker, int row, int col)
{
	for(int i=0;i < pCore->n_mobs;i++)
	{
		if(pCore->sprites[pCore->mobs[i]]->getPosition()->getRow()==row && pCore->sprites[pCore->mobs[i]]->getPosition()->getCol()==col)
		{
			if(pCore->sprites[pCore->mobs[i]]->isAlive())
				return pCore->sprites[pCore->mobs[i]];
		}
	}

	return NULL;


}

bool Battlehelper::CheckValidMove(vector<Position> p, int row, int col)
{
	for(int i = 0; i < static_cast<int>(p.size()); i++)
	{
		if(p[i].getRow()==row &&p[i].getCol()==col)
			return true;
	}

	return false;
}


void Battle::InitialPosition()
{
	/*
	int n;//for minimum possible moves
	n=0;
	while(n<3)
	{
		int radius=1;
		if(map[pCore->sprites[0]->getPosition()->getRow()+radius][pCore->sprites[0]->getPosition()->getCol()]!="w"
			&&map[pCore->sprites[0]->getPosition()->getRow()+radius][pCore->sprites[0]->getPosition()->getCol()]!="#"
			&&map[pCore->sprites[0]->getPosition()->getRow()+radius][pCore->sprites[0]->getPosition()->getCol()]!="o")

	}*/


	/* just hard coded the other player's positions here */
	pCore->sprites[1]->getPosition()->moveCardinalDirection(pCore->sprites[0]->getPosition()->getRow()+1,pCore->sprites[0]->getPosition()->getCol()-1,pCore->sprites[1]->getCurrentMap());
	pCore->sprites[2]->getPosition()->moveCardinalDirection(pCore->sprites[0]->getPosition()->getRow()-1,pCore->sprites[0]->getPosition()->getCol()-1,pCore->sprites[2]->getCurrentMap());
	pCore->sprites[3]->getPosition()->moveCardinalDirection(pCore->sprites[0]->getPosition()->getRow()-2,pCore->sprites[0]->getPosition()->getCol(),pCore->sprites[3]->getCurrentMap());
}
Battle::Battle()
{
	turncounter = 0;
	whichpicking = 0;
	currentCharacter = nullptr;
	int playercounter = 0;
	int mobcounter = 0;
	state = 1;
	pick = nullptr;
	inner_c = 0;
	step = 4;
	player_death = 0;
	availablemoves.clear();
	deading_frame_counter = 0;
	move_frame_counter = 0;
	attack_frame_counter = 0;
	temp_cursorX = pCore->sprites[0]->getPosition()->getX();
	temp_cursorY = pCore->sprites[0]->getPosition()->getY();
	MouseX = temp_cursorX;
	MouseY = temp_cursorY;
}

Battle::Battle(Core* p, char** m) : pCore(p), map(m) {
	pBattleHelper = new Battlehelper(pCore, map);
	turncounter = 0;
	whichpicking = 0;
	currentCharacter = nullptr;
	int playercounter = 0;
	int mobcounter = 0;
	state = 1;
	pick = nullptr;
	inner_c = 0;
	step = 4;
	player_death = 0;
	availablemoves.clear();
	deading_frame_counter = 0;
	move_frame_counter = 0;
	attack_frame_counter = 0;
	temp_cursorX = pCore->sprites[0]->getPosition()->getX();
	temp_cursorY = pCore->sprites[0]->getPosition()->getY();
	MouseX = temp_cursorX;
	MouseY = temp_cursorY;
}

void Battle::BattleLoopUpdate()
{
	switch(state)
	{
		case 1:	{ //initalize the battle information
				InitialPosition();//put 4 character to the position within 1 tile radius
				mob_reaction=0;
				player_reaction=0;
				mob_turn=false;
				state=2;
				break;
			}
		case 2: {//new round, check each team/party's dead or alive. calculating the average reaction value to determing which party's turn
				turncounter=0;
				for(int i=0;i<pCore->n_mobs;i++) {
					if(!bp.Checkalive(pCore->sprites[pCore->mobs[i]])) {// check the mob if it is dead  
						pCore->n_mobs--;
						pCore->mobs.erase(pCore->mobs.begin()+i);
					} else {
						mob_reaction+=pCore->sprites[pCore->mobs[i]]->getMajor()->getDexterity();
					}
				}
				if(pCore->n_mobs<=0) {
					pCore->mode=1;//victory return exploring mode
					//need calculate experience function
						return;
				}

				for(int i=0;i<4;i++) {
					if (!bp.Checkalive(pCore->sprites[i])) {
						player_death++;
					} else {
						player_reaction += pCore->sprites[i]->getMajor()->getDexterity();
					}
				}
				if(player_death==4) {
					pCore->mode=5;//game over
					return;
				}

				//calculating the average reaction value
				mob_reaction = mob_reaction / pCore->n_mobs;
				player_reaction = player_reaction / (4 - player_death);

				if (player_reaction < mob_reaction) {// if mob party's reaction value is larger, then mob's turn
					mob_turn = true;
					mobcounter = 0;
					state = 4;// go to mob's turn
				} else {
					mob_turn = false;
					playercounter = 0;
					state = 3; //go to player's turn
				}
				break;
			}

		case 3: {//player party's turn, put player's item and skills into the droplist
		mob_turn = false;
		turncounter++;
		if (turncounter <= 2) {//if two parties have not finished
			if (playercounter < 4 - player_death) {//if player party has not finished: number of played/moved less than number of alive
				currentCharacter=pCore->sprites[playercounter];
				pCore->currentPlayer=playercounter;
				//PrintStatus(currentCharacter);//call user interface function display;

  			for(int i=0;i<pCore->sprites[pCore->currentPlayer]->getInventory()->getNrOfitems();i++)	{
				LRESULT pos=SendMessage(pCore->hwndItemListBox,CB_ADDSTRING ,0,(LPARAM)(pCore->sprites[pCore->currentPlayer]->getInventory()->getItemList()[i]->getName().c_str()));  
				SendMessage(pCore->hwndItemListBox, CB_SETITEMDATA ,pos,(LPARAM)i);
			}
			for(int i=0;i<pCore->sprites[pCore->currentPlayer]->getMajor()->getNrOfSkill() ;i++) {
				LRESULT pos=SendMessage(pCore->hwndSkillDropBox,CB_ADDSTRING ,0,(LPARAM)(pCore->sprites[pCore->currentPlayer]->getMajor()->getSkillList()[i]->getName().c_str()));
				SendMessage(pCore->hwndSkillDropBox, CB_SETITEMDATA ,pos,(LPARAM)i);
			}
			state=5;//goto pick choice state
			} else {
				state = 4;//player party has finished this round goto mob party
			}
		} else {
			state = 2;//if two parties have finished then goto initialize new round state
		}
		break;
		}

		case 4: {//mob party's turn
				mob_turn = true;
				turncounter++;
				if (turncounter <= 2) {//if two parties have not finished
					if(mobcounter<pCore->n_mobs) {//if mob party has not finished
						currentCharacter=pCore->sprites[pCore->mobs[mobcounter]];
						//PrintStatus(currentCharacter);
						state=14;//mob action
					} else {
						state = 3;//mob party has finished this round goto player party
					}
				} else {
					state = 2;//if two parties have finished then goto initialize new round state
				}
				break;
			}
		case 5: {//pick mode
			userchoice=0;
			state=50;
			break;
		}
		case 50: {
			if(userchoice!=0) {
				state=6;
			}
			break;
			}
		case 6: {
			switch(userchoice) {
				case 0://empty slot
					break;
				case 1: {//attack	
					//shadeAvailable(bp.availableAttack(currentCharacter));
					whichpicking=1;//attack picking
					state=7;
					break;
				}
				case 2: {//move	
					availablemoves=bp.availableMove(currentCharacter);
					//shadeAvailable(availablemoves);
					whichpicking=2;//move picking
					state=7;
					break;
				}
				case 3: {//item	
					//......//
					//itemindex is global intger for item combobox choosen
					bp.DealItem((currentCharacter->getInventory()->getItemList())[pCore->itemindex],currentCharacter,pCore->sprites[5]);
					//state=xx; using item animation
					state=3;
					break;
			}
			case 4: {//skill
				//......//
				//state=xx; using skill animation
				bp.DealSkill((currentCharacter->getMajor()->getSkillList())[pCore->skillindex],currentCharacter,pCore->sprites[5]);
				state=3;
				break;
			}
			default:
				break;
		}
			}//end of case 6:


		case 7:	{ //picking position status, wait(keep re-entering) this state until the mouse click a different places
				if(MouseX != temp_cursorX || MouseY!= temp_cursorY) {
					pick=new Position;
					pick->moveCardinalDirectionPixel(MouseX, MouseY,currentCharacter->getCurrentMap());
				} else {
					pick = nullptr;
				}
				if(pick != nullptr)	{
					if (whichpicking == 1) { //attack picking
						state = 8;// go to attacking state
					}
					else if (whichpicking == 2) {//move picking
						state = 9;// go to moving state
						//else if(whichpicking==3)//item(attacking) picking
							//state=
					} else {
						MessageBox(NULL, "Should not be here (waiting picking)!", "Fail", MB_ICONEXCLAMATION | MB_OK);
					}
				}
				break;
			}

		case 8: {//validate attack		
			whichpicking = 0;
			beatenMob = nullptr;
			whichpicking = 0;
			beatenMob = bp.CheckValidAttack(currentCharacter,pick->getRow(),pick->getCol());
			//delete pick;

			if(beatenMob != nullptr) {
				state=10;//attacking animation
				attack_frame_counter=0;
			} else {
				MessageBox(NULL,"You can't attack there!","Fail",MB_ICONEXCLAMATION | MB_OK);
				state = 7;//go back to wait picking state
				delete pick;
				pick = nullptr;
				whichpicking = 1;//attack button is pressed, go back to picking state to wait pick correct attack target
			}					
			break;			
		}

		case 9: {//validate move:
			bool flag = false;
			whichpicking = 0;
			flag = bp.CheckValidMove(availablemoves,pick->getRow(),pick->getCol());
			if (flag == false) {
				MessageBox(NULL,"You can't move there!","Fail",MB_ICONEXCLAMATION | MB_OK);
				state = 7;
				delete pick;
				pick = nullptr;
				whichpicking = 2;//move button is pressed, go back to picking state to wait pick correct move target
			} else {
				availablemoves.clear();
				state=11; //move animation+A*
			}				
			break;
		}

		case 10: {//attack animation frame control: we only need one time animation
			idle_disable=true;
			if (attack_frame_counter < 6) {
				currentCharacter->setCurrentSprite(pCore->player_female_attacking[currentCharacter->getPosition()->getDir()][attack_frame_counter]);
				attack_frame_counter++;
			} else {
						//PrintBattleStatus(hdc,bp.DealDamage(currentCharacter,beatenMob));
						if(!bp.Checkalive(beatenMob)) {//mob is dead
							attack_frame_counter=0;//resume animation frame to the starting frame
							idle_disable = false;//
							delete pick;
							pick = nullptr;
							state = 13;//deading animation;
							deading_frame_counter = 0;
							currentDeadingChar = beatenMob;
							beatenMob = nullptr;
						} else {
							beatenMob = nullptr;
							attack_frame_counter=0;//resume animation frame to the starting frame
							idle_disable = false;//
							delete pick;
							pick = nullptr;
							if (mob_turn) {
								mobcounter++;//change to the next mob of mob party
								state = 4;//go back to the mob party turn control
							} else {
								playercounter++;//change to the next player of our party
								state=3;//go back to player party turn control
							}
						}
					}
					break;
				}

			case 11: {//call a* function to find a path
					FindPath(pick->getRow(),pick->getCol());
					if(path == nullptr) {
						MessageBox(NULL,"Should not be empty path of battle astar","Fail",MB_ICONEXCLAMATION | MB_OK);
						exit(1);
					}
					state = 12;
					break;
				}

			case 12:// move animation control and update move x,y coordinates
				{
					idle_disable=true;
					if(move_frame_counter<6)
					{
						currentCharacter->setCurrentSprite(pCore->player_female_moving[currentCharacter->getPosition()->getDir()][move_frame_counter]);
						move_frame_counter++;
					}
					else
						move_frame_counter=0;

					//decompose the tile coordinates into 1/4 pixel coordinates
						int Rd,Cd;
						if(p != NULL)
						{
								  if(inner_c%step == 0)
									{
										  inner_c = 0;
										  preRow = p->info.y;
										  preCol = p->info.x;
										  temp=p;
										  p = p->next;
	  
										  currRow = p->info.y;
										  currCol = p->info.x;
	  
										  Rd = (currRow - preRow)*40/step;
										  Cd = (currCol - preCol)*40/step;
	  
										  delete temp;
										  temp=NULL;
									}
								  currentCharacter->getPosition()->moveCardinalDirectionPixel(currentCharacter->getPosition()->getX() + Cd, currentCharacter->getPosition()->getY() + Rd,currentCharacter->getCurrentMap());
								  inner_c++;

						}
						else
						{
							delete pick;
							pick=NULL;
							move_frame_counter=0;//resume animation frame to the starting frame
							idle_disable=false;//
							if(mob_turn)
							{
								mobcounter++;//change to the next mob of mob party
								state=4;//go back to the mob party turn control
							}
							else
							{
								playercounter++;//change to the next player of our party
								state=3;//go back to player party turn control
							}
						}
					break;
				}

			case 13: // deading animation
				{
					idle_disable=true;
					if(deading_frame_counter<6)
					{
						currentDeadingChar->setCurrentSprite(pCore->mob_dead[currentCharacter->getPosition()->getDir()][deading_frame_counter]);
						deading_frame_counter++;
					}
					else
					{
						idle_disable=false;
						deading_frame_counter=0;
						currentDeadingChar=NULL;

						if(mob_turn)
							{
								mobcounter++;//change to the next mob of mob party
								state=4;//go back to the mob party turn control
							}
							else
							{
								playercounter++;//change to the next player of our party
								state=3;//go back to player party turn control
							}
					}

					break;
				}


			case 14://mob action
				{
					
					if(currentCharacter->getCurrentHP()/(float)currentCharacter->getMajor()->getHealth()>0.33)
					{
						Character* target=NULL;
						int i=0;
						//choose the first alive player
						while(target==NULL)
						{
							if(pCore->sprites[i]->isAlive())
								target=pCore->sprites[i];
							i++;
						}


						for(int i=0;i<4;i++)	//choose the loweset hp "alive" player to attack
						{
							if(pCore->sprites[i]->getCurrentHP()<target->getCurrentHP())
							{
								if(pCore->sprites[i]->isAlive())
									target=pCore->sprites[i];
							}
						}

						//if not in the attacking range then move
						if((currentCharacter->getAttackRange())<AttackDistance(target->getPosition()->getRow(),target->getPosition()->getCol()))
							{
								availablemoves.clear();
								availablemoves=bp.availableMove(currentCharacter);
								double close=10000;
								int index;
								for(int i = 0; i < static_cast<int>(availablemoves.size()); i++)
								{
									double temp=sqrt(pow((availablemoves[i].getCol()-target->getPosition()->getCol()),2.0)+pow((availablemoves[i].getRow()-target->getPosition()->getRow()),2.0));
									if(close>temp)
									{
										close=temp;
										index=i;
									}
								}
								FindPath(availablemoves[index].getRow(),availablemoves[index].getCol());
								if(path==NULL)
								{
									MessageBox(NULL,"Should not be empty path of battle astar","Fail",MB_ICONEXCLAMATION | MB_OK);
									exit(1);
								}
								state=15;
							}
						else
							{
								beatenPlayer=target;
								state=16;
							}
					}
					else //if mob hp is lower than 1/3
					{

								Character* target=NULL;
						int i=0;
						//choose the first alive player
						while(target==NULL)
						{
							if(pCore->sprites[i]->isAlive())
								target=pCore->sprites[i];
							i++;
						}

						int temp=target->getAttackDamage();
						int tempindex=i;
						for(int i=0;i<4;i++)	//choose the loweset hp "alive" player to attack
						{
							if(pCore->sprites[i]->getAttackDamage()>temp)
							{
								if(pCore->sprites[i]->isAlive())
								{
									temp=pCore->sprites[i]->getAttackDamage();
									tempindex=i;
								}
							}
						}
						target=pCore->sprites[i];

								availablemoves.clear();
								availablemoves=bp.availableMove(currentCharacter);
								double close=0;
								int index;

								for(int i = 0; i < static_cast<int>(availablemoves.size()); i++)
								{



									double temp=sqrt(pow((availablemoves[i].getCol()-target->getPosition()->getCol()),2.0)+pow((availablemoves[i].getRow()-target->getPosition()->getRow()),2.0));
									if(close<temp)
									{
										close=temp;
										index=i;
									}
								}
								FindPath(availablemoves[index].getRow(),availablemoves[index].getCol());
								if(path==NULL)
								{
									MessageBox(NULL,"Should not be empty path of battle astar","Fail",MB_ICONEXCLAMATION | MB_OK);
									exit(1);
								}
								state=15;


					}

					break;
				}
			case 15:// move animation control and update move x,y coordinates
				{
					idle_disable=true;
					if(move_frame_counter<6)
					{
						currentCharacter->setCurrentSprite(pCore->mob_moving[currentCharacter->getPosition()->getDir()][move_frame_counter]);
						move_frame_counter++;
					}
					else
						move_frame_counter=0;

					//decompose the tile coordinates into 1/4 pixel coordinates
						int Rd,Cd;
						if(p != NULL)
						{
								  if(inner_c%step == 0)
									{
										  inner_c = 0;
										  preRow = p->info.y;
										  preCol = p->info.x;
										  temp=p;
										  p = p->next;
	  
										  currRow = p->info.y;
										  currCol = p->info.x;
	  
										  Rd = (currRow - preRow)*40/step;
										  Cd = (currCol - preCol)*40/step;
	  
										  delete temp;
										  temp=NULL;
									}
								  currentCharacter->getPosition()->moveCardinalDirectionPixel(currentCharacter->getPosition()->getX() + Cd, currentCharacter->getPosition()->getY() + Rd,currentCharacter->getCurrentMap());
								  inner_c++;

						}
						else
						{
							move_frame_counter=0;//resume animation frame to the starting frame
							idle_disable=false;//
							if(mob_turn)
							{
								mobcounter++;//change to the next mob of mob party
								state=4;//go back to the mob party turn control
							}
							else
							{
								playercounter++;//change to the next player of our party
								state=3;//go back to player party turn control
							}
						}
					break;
				}



			case 16://attack animation frame control: we only need one time animation
				{
					idle_disable=true;
					if(attack_frame_counter<6)
					{
						currentCharacter->setCurrentSprite(pCore->mob_attacking[currentCharacter->getPosition()->getDir()][attack_frame_counter]);
						attack_frame_counter++;
					}
					else
					{
						//PrintBattleStatus(hdc,bp.DealDamage(currentCharacter,beatenPlayer));
						if(!bp.Checkalive(beatenPlayer))//Player is dead
						{
							attack_frame_counter=0;//resume animation frame to the starting frame
							idle_disable=false;//;
							state=17;//deading player animation;
							deading_frame_counter=0;
							currentDeadingChar=beatenPlayer;
						}
						else
						{
							attack_frame_counter=0;//resume animation frame to the starting frame
							idle_disable=false;//
							if(mob_turn)
							{
								mobcounter++;//change to the next mob of mob party
								state=4;//go back to the mob party turn control
							}
							else
							{
								playercounter++;//change to the next player of our party
								state=3;//go back to player party turn control
							}
						}
					}
					break;
				}

			case 17: // deading animation
				{
					idle_disable=true;
					if(deading_frame_counter<6)
					{
						currentDeadingChar->setCurrentSprite(pCore->player_female_dead[currentDeadingChar->getPosition()->getDir()][deading_frame_counter]);
						deading_frame_counter++;
					}
					else
					{
						idle_disable=false;
						deading_frame_counter=0;
						currentDeadingChar=NULL;

						if(mob_turn)
							{
								mobcounter++;//change to the next mob of mob party
								state=4;//go back to the mob party turn control
							}
							else
							{
								playercounter++;//change to the next player of our party
								state=3;//go back to player party turn control
							}
					}

					break;
				}






	}
				

	/*
	// update the rest of character's idle animation frame

	for(int i=0;i<pCore->n_mobs;i++)
	{
	if(!bp.Checkalive(pCore->sprites[pCore->mobs[i]]))// check the mob if it is dead  
	{
		if(pCore->sprites[pCore->mobs[i]]!=currentDeadingChar)
			pCore->sprites[pCore->mobs[i]]->setCurrentSprite(pCore->mob_dead[currentDeadingChar->getPosition()->getDir()][6]);//always last frame of deading
	}
	else
	{


		if(pCore->sprites[pCore->mobs[i]]==currentCharacter)//? can we compare two pointer?
		{
			if(idle_disable!=true)//idle
			{
				if(currentCharacter->getIdle_Frame_counter()<6)
				{
					currentCharacter->setCurrentSprite(pCore->player_stopping[currentDeadingChar->getPosition()->getDir()][currentCharacter->getIdle_Frame_counter()]);
					currentCharacter->increIdle_Frame_counter();
				}
				else
					currentCharacter->resetIdle_Frame_counter();
			}

		}
		else
		{
				if(pCore->sprites[pCore->mobs[i]]->getIdle_Frame_counter()<6)
				{
					pCore->sprites[pCore->mobs[i]]->setCurrentSprite(pCore->player_stopping[currentDeadingChar->getPosition()->getDir()][pCore->sprites[pCore->mobs[i]]->getIdle_Frame_counter()]);
					pCore->sprites[pCore->mobs[i]]->increIdle_Frame_counter();
				}
				else
					pCore->sprites[pCore->mobs[i]]->resetIdle_Frame_counter();
		}
	 }
	}



			for(int i=0;i<4;i++)
			{
				if(!bp.Checkalive(pCore->sprites[i]))
				{
				if(pCore->sprites[i]!=currentDeadingChar)
					pCore->sprites[i]->setCurrentSprite(pCore->player_dead[currentDeadingChar->getPosition()->getDir()][6]);//always last frame of deading
				}
				else
				{
					if(pCore->sprites[i]==currentCharacter)//? can we compare two pointer?
					{
						if(idle_disable!=true)//idle
						{
									if(currentCharacter->getIdle_Frame_counter()<6)
									{
										currentCharacter->setCurrentSprite(pCore->player_stopping[currentDeadingChar->getPosition()->getDir()][currentCharacter->getIdle_Frame_counter()]);
										currentCharacter->increIdle_Frame_counter();
									}
									else
										currentCharacter->resetIdle_Frame_counter();
						}
					}
					else
					{
						if(pCore->sprites[i]->getIdle_Frame_counter()<6)
						{
							pCore->sprites[i]->setCurrentSprite(pCore->player_stopping[pCore->sprites[i]->getPosition()->getDir()][pCore->sprites[i]->getIdle_Frame_counter()]);
							pCore->sprites[i]->increIdle_Frame_counter();
						}
						else
							pCore->sprites[i]->resetIdle_Frame_counter();
					}
				}
			}
			*/

}


nodeptr Battle::FindPath(int row, int col)
{

	int startX, startY, endX, endY;

  
	if (currentCharacter->getPosition()->getCol()-currentCharacter->getMoveRange() < 0) startX = 0;
	else startX = currentCharacter->getPosition()->getCol()-currentCharacter->getMoveRange();
	if (currentCharacter->getPosition()->getRow()-currentCharacter->getMoveRange() < 0) startY = 0;
	else startY = currentCharacter->getPosition()->getRow()-currentCharacter->getMoveRange();
	if (currentCharacter->getPosition()->getCol()+currentCharacter->getMoveRange() > 8) endX = 8;
	else endX = currentCharacter->getPosition()->getCol()+currentCharacter->getMoveRange();
	if (currentCharacter->getPosition()->getRow()+currentCharacter->getMoveRange() > 7) endY = 7;
	else endY = currentCharacter->getPosition()->getRow()+currentCharacter->getMoveRange();
  

  char** screen = new char*[endY - startY];
  
  for(int i=0;i<endY - startY;i++)
    {
      screen[i]=new char[endX - startX];
    }
  
  for (int i=0; i<endY - startY; i++)
    {
      for(int j=0; j<endX - startX; j++)
		{
		screen[i][j] = map[i+startY][j+startX];
		}
    }

  
  path = astar1_search(screen, endY - startY, endX - startX,row, col,currentCharacter->getPosition()->getRow(), currentCharacter->getPosition()->getCol());
  p=path;
  for(int i=0;i<endY - startY;i++)
    {
      delete []screen[i];
    }
  delete []screen;
  return p;

}

int Battle::AttackDistance(int row, int col)
{

	int startX, startY, endX, endY;

  
	if (currentCharacter->getPosition()->getCol()-10 < 0) startX = 0;
	else startX = currentCharacter->getPosition()->getCol()-10;
	if (currentCharacter->getPosition()->getRow()-10 < 0) startY = 0;
	else startY = currentCharacter->getPosition()->getRow()-10;
	if (currentCharacter->getPosition()->getCol()+10 > 8) endX = 8;
	else endX = currentCharacter->getPosition()->getCol()+10;
	if (currentCharacter->getPosition()->getRow()+10 > 7) endY = 7;
	else endY = currentCharacter->getPosition()->getRow()+10;
  

  char** screen = new char*[endY - startY];
  
  for(int i=0;i<endY - startY;i++)
    {
      screen[i]=new char[endX - startX];
    }
  
  for (int i=0; i<endY - startY; i++)
    {
      for(int j=0; j<endX - startX; j++)
		{
		screen[i][j] = map[i+startY][j+startX];
		}
    }

  
  path = astar1_search(screen, endY - startY, endX - startX,row, col,currentCharacter->getPosition()->getRow(), currentCharacter->getPosition()->getCol());
  int n=0;
  while(path!=NULL)
  {
	  n++;
	  temp=path;
	  delete temp;
	  temp=NULL;
	  p=p->next;
  }

  for(int i=0;i<endY - startY;i++)
    {
      delete []screen[i];
    }
  delete []screen;
  return n;
}
