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
#include <vector>
#include <queue>
#include <fstream>
#include <string>
#include <ctime>
#include <Wincodec.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "resource.h"
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


using namespace std;


int WINAPI Core::run() {
	pGraphic = new Graphic(this);
	initialGame();
	initialWindow();
	pBattle = new Battle(this, sprites[0]->getCurrentMap()->getMap());//battle need sprites[0]->, so need initalGame() first to push some sprites in the vector
	MSG msg;
	while(true) {
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(msg.message == WM_QUIT)
				break;
		}

		GameControl();
	}
	return (int)msg.wParam ;
}
void Core::GameControl() {

	//mode==0: starting animation: background introduction, mode=0 is initialized in initial()
	if (mode == 1)    // exploration mode
	{
		ShowWindow(hwndJornalBT,SW_SHOW);
		ShowWindow(hwndMapBT,SW_SHOW);
		ShowWindow(hwndStatusBT,SW_SHOW);
		ShowWindow(hwndItemListBox,SW_SHOW);
		
		teampop->TeamPoping(sprites, f6pressed);

		for (auto sprite : sprites) {
			if (singlestate == 1 || singlestate == 3) {// animation mode
				if (sprite->getMajor()->getName() == "Engineer" || sprite->getMajor()->getName() == "Guardsman" || sprite->getMajor()->getName() == "Chemist" || sprite->getMajor()->getName() == "Philosopher") {
					continue;
				}
			} else {//four people mode
				if (singlestate == 4 && (sprite->getMajor()->getName() == "Guardsman" || sprite->getMajor()->getName() == "Chemist" || sprite->getMajor()->getName() == "Philosopher")) {
					continue;
				}
			}
			sprite->getWalkEngine()->Control();
		}
		//pExplore->Control();
		pGraphic->DrawScreen(hChild,sprites[0]);
	}
	else if (mode == 2)	// combat mode
	{
		pBattle->BattleLoopUpdate();
	}
	else if (mode == 3)	// in menu
	{
		if(whichbutton=="journal")
		{
			pGraphic->DrawScreen(hChild,current_journal, hbitmap[0],175,50, sprites[0]->getCamera()->getLeft(),sprites[0]->getCamera()->getTop());
		}
		else if(whichbutton=="map")
		{
			//mapID==1:jeb1.txt      mapID==2:jeb2.txt      mapID==3:jeb3.txt
			if(sprites[0]->getCurrentMap()->getMapID() == 1||sprites[0]->getCurrentMap()->getMapID() == 2||sprites[0]->getCurrentMap()->getMapID() == 3) {
				currentX=190;
				currentY=162;
			} else if(sprites[0]->getCurrentMap()->getMapID() == 4) {//mapID==4:commons.txt
				currentX=72;
				currentY=500;
			} else {//mapID==0:main_map.txt
				currentX=sprites[0]->getPosition()->getX()/21;
				currentY=sprites[0]->getPosition()->getY()/21;
			}
			pGraphic->DrawMapMode(hChild, sprites[0], hbitmap[232], hbitmap[231], hbitmap[26], 362, 0, targetX+362,targetY+0,currentX+362,currentY+0);
		} else if (whichbutton=="stat") {
			pGraphic->ShowStat(hChild);
		}
	} else if (mode == 4) {// dialogue mode
		for (auto sprite : sprites) {
			sprite->getCurrentMap()->getShoeSound(sprite->getShoeType())->Stop();
		}
		dialogueupdate(hwnd);
		pGraphic->DrawScreen(hChild, sprites[0]);	
	}
}

void Core::dialogueupdate(HWND hwnd)
{
	if (mouseclick)//everytime mouse left click->set mouseclick=true, then in this dialog update
	{
		mouseclick = false;//set mouseclick=false wait another mouse left click to set it to true to come in again. if no left click, it will not come in this dialogue update.
		if (dindex == 0) {//show the first monologue of the player
			mode = 1;//changing to exploring mode
			dindex++;//go to dindex=1 next dialogue;
			currentTaskID = 1;
			static_cast<NPC*>(sprites[4])->setTaskID(1);
		}
		if (dindex == 4) // practice moving  dialogue 0-3: welcome and let player to try go around a bit for move control
		{
			//show the exploring mode menu
			currentTaskID = 2;
			static_cast<NPC*>(sprites[4])->setTaskID(2);
			mode = 1;//changing to exploring mode
			dindex++;//go to dindex=5 next dialogue;
			antiAround = { true, 4 };
			//static_cast<Player*>(sprites[0])->getWalkEngine()->setAntiAround(true, 4);
		}
		else if (dindex == 5) 
		{

			mode = 1;
			//change the location of tour guide
			//sprites[4]->getPosition()->moveCardinalDirectionPixel(2527, 904, sprites[4]->getCurrentMap());
			static_cast<NPC*>(sprites[4])->getWalkEngine()->SetWalkTarget(2527, 904); //let tour guide walk to the next point, not teleport
			static_cast<NPC*>(sprites[4])->setDialogtrigger(false);
			dindex = 6;
			currentTaskID = 3;
			static_cast<NPC*>(sprites[4])->setTaskID(3);
			//experience add 2;
			static_cast<Player*>(sprites[0])->addExperience(2);

			current_journal =  journal[1];

		}
		else if (dindex == 9) //get Dagger and Pistol in dialogue[10]
		{
			dindex++;
			//add dagger and pistol into item
			Item* dagger=new Item("Dagger",20,567,5,3,5,1);
			Item* pistol=new Item("Pistol",80,234,10,3,10,5);
			sprites[0]->getInventory()->addItem(this,dagger);
			sprites[0]->getInventory()->addItem(this,pistol);
		}
		else if (dindex == 12) // move to arena
		{
			dindex++;
			mode = 1;//next enter mode 4 dialogue mode will be in commons arena, it will loading tourguide13.bmp

			// change the location of tour guide to arena (commons1);
			//sprites[4]->getPosition()->moveCardinalDirectionPixel(1458, 481,pMapGraph->getMap("commons"));
			// change the map for tour guide
			//sprites[4]->setCurrentMap(pMapGraph->getMap("commons"));
			//sprites[4]->getCamera()->setCamera(pMapGraph->getMap("commons"), 1458, 481);
			sprites[4]->moveTo(1458, 481, pMapGraph, pMapGraph->getMap("commons"));
			static_cast<NPC*>(sprites[4])->setDialogtrigger(false);
			//experience add 5;
			static_cast<Player*>(sprites[0])->addExperience(5);
			currentTaskID = 4;
			static_cast<NPC*>(sprites[4])->setTaskID(4);
			current_journal = journal[2];
			targetX=77;
			targetY=487;
		}
		else if (dindex == 13) 
		{
			dindex = 16;	//first dialoge in arena (dindex=13, tourguide13.bmp), then move to dindex = 16 (tourguide16.bmp)
		}
		else if (dindex == 16) // second dialogue in commons: now try your hand defeating a fecious creature...
		{
			dindex++;
			mode = 1;
 
			// make the tour guide disappear
			//sprites[4]->setAlive(false);
			//tour guide will move out of commons to next targte place
			sprites[4]->moveTo(469,9918, pMapGraph, pMapGraph->getMap("main_map"));
			static_cast<NPC*>(sprites[4])->setDialogtrigger(false);
			sprites[6]->setAlive(true);
			sprites[6]->getPosition()->moveCardinalDirectionPixel(1481, 9857,pMapGraph->getMap("main_map"));//now the current_map is main_map
			int row = sprites[6]->getPosition()->getRow();
			int col = sprites[6]->getPosition()->getCol();
			sprites[6]->getWalkEngine()->setOccupiedTile(row, col);
			sprites[6]->getCurrentMap()->getMap()[row][col] = 'w';
			currentTaskID = 5;
			static_cast<NPC*>(sprites[6])->setTaskID(5);
			static_cast<NPC*>(sprites[4])->clearTaskID();
		}
		else if (dindex == 19) // 17,18,19, three dialogues with policeman
		{

			mode = 1;	//combat mode;
			dindex = 21;
			// make the police guard disappear
			sprites[6]->moveTo(622, 217, pMapGraph, pMapGraph->getMap("jeb3"));
			static_cast<NPC*>(sprites[6])->setDialogtrigger(false);
			currentMonitoredNPC = 6;
			currentTaskID = 6;
			static_cast<NPC*>(sprites[4])->setTaskID(6);
			static_cast<NPC*>(sprites[6])->clearTaskID();
			//new target for meet tour guide
			targetX = 30;
			targetY = 470;
		}
		else if (dindex == 21) // move northwest to find chemistry booth
		{
			dindex++;
			mode = 1;  
			//change the location of tour guide to chemistry booth;
			static_cast<NPC*>(sprites[4])->getWalkEngine()->SetWalkTarget(858, 9111);
			static_cast<NPC*>(sprites[4])->setDialogtrigger(false);
			//experience add 10;
			static_cast<Player*>(sprites[0])->addExperience(10);
			current_journal = journal[3];
			targetX=45;
			targetY=438;
			currentTaskID = 7;
			static_cast<NPC*>(sprites[4])->setTaskID(7);
		}
		else if (dindex == 22)
		{

			dindex = 23;
			// get two grenades into items
			Item* g1=new Item("Grenades",20,456,23,1,20,5);
			Item* g2=new Item("Grenades",20,500,23,1,20,5);;
			sprites[0]->getInventory()->addItem(this,g1);
			sprites[0]->getInventory()->addItem(this,g2);
		}
		else if (dindex == 27)
		{
			dindex = 15;
		}
		else if (dindex == 15) dindex = 28;
		else if (dindex == 28)
		{
			dindex++;
			mode = 1;

			// change the location of tour guide to natrual philosopher booth;.
			static_cast<NPC*>(sprites[4])->getWalkEngine()->SetWalkTarget(2297, 4103);
			static_cast<NPC*>(sprites[4])->setDialogtrigger(false);
			//experience add 15;
			static_cast<Player*>(sprites[0])->addExperience(15);
			current_journal = journal[4];
			targetX=90;
			targetY=206;
			currentTaskID = 8;
			static_cast<NPC*>(sprites[4])->setTaskID(8);
		}
		else if (dindex == 32)
		{
			dindex = 37;
			//experience add 20;
			static_cast<Player*>(sprites[0])->addExperience(20);
			current_journal = journal[5];
			targetX=187;
			targetY=167;
		}
		else if (dindex == 39)
		{
			dindex++;
			mode = 1;
			//make the tour guide disappear.
			sprites[4]->setAlive(false);
			int row = sprites[4]->getPosition()->getRow();
			int col = sprites[4]->getPosition()->getCol();
			sprites[4]->getWalkEngine()->setOccupiedTile(row, col);
			sprites[4]->getCurrentMap()->getMap()[row][col] = '.';
			//place Jint Clevery in JEB 2end floor.
			sprites[5]->setAlive(true);
			sprites[5]->setCurrentMap(pMapGraph->getMap("jeb2"));
			sprites[5]->getPosition()->moveCardinalDirectionPixel(186, 1650,pMapGraph->getMap("jeb2"));
			sprites[5]->getCamera()->setCamera(pMapGraph->getMap("jeb2"), 144, 1586);
			row = sprites[5]->getPosition()->getRow();
			col = sprites[5]->getPosition()->getCol();
			sprites[5]->getWalkEngine()->setOccupiedTile(row, col);
			sprites[5]->getCurrentMap()->getMap()[row][col] = 'w';
			currentTaskID = 9;
			static_cast<NPC*>(sprites[5])->setTaskID(9);
			static_cast<NPC*>(sprites[4])->clearTaskID();
		}
		else if (dindex == 40)
		{
			dindex = 14;
		}
		else if (dindex == 14) dindex = 41;
		else if (dindex == 41) dindex = 45;
		else if (dindex == 73) 
		{
			mode = 1;
			//make Jint disappear;
			//sprites[5]->setAlive(false);
			sprites[5]->moveTo(166, 290, pMapGraph, pMapGraph->getMap("commons"));
			currentMonitoredNPC = 5;
			static_cast<NPC*>(sprites[5])->setDialogtrigger(false);
			//experience add 25;
			static_cast<Player*>(sprites[0])->addExperience(25);
			current_journal = journal[6];
			targetX=-40;
			targetY=-40;
			static_cast<NPC*>(sprites[5])->clearTaskID();
		}
		else if ((dindex >= -1 && dindex <=3) || (dindex >= 6 && dindex < 9) || (dindex > 9 && dindex<= 11) ||  (dindex <= 18 && dindex >= 17) || (dindex <= 26 && dindex >= 23) || (dindex <= 31 && dindex >= 29) || dindex == 33 || (dindex <= 38 && dindex >= 37) || (dindex <= 72 && dindex >= 45))
			dindex++;
		current_dialogue = ".//Dialogue//"+dialogue[dindex];
		picture=(HBITMAP)LoadImage((HINSTANCE)NULL,current_dialogue.c_str(),IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
		InvalidateRect(hwnd,NULL,true);
	}
}

void Core::dialogue_initial()
{
    dialogue[0] = "Introduction.bmp";
    dialogue[1] = "tourguide1.bmp";
    dialogue[2] = "tourguide2.bmp";
    dialogue[3] = "tourguide3.bmp";
    dialogue[4] = "tourguide4.bmp";
    dialogue[5] = "tourguide5.bmp";
    dialogue[6] = "tourguide6.bmp";
    dialogue[7] = "tourguide7.bmp";
    dialogue[8] = "tourguide8.bmp";
    dialogue[9] = "tourguide9.bmp";
    dialogue[10] = "tourguide10.bmp";
    dialogue[11] = "tourguide11.bmp";
    dialogue[12] = "tourguide12.bmp";
    dialogue[13] = "tourguide13.bmp";
    dialogue[14] = "tourguide14.bmp";
    dialogue[15] = "tourguide15.bmp";
    dialogue[16] = "tourguide16.bmp";
    dialogue[17] = "pg1.bmp";
    dialogue[18] = "Information1.bmp";
    dialogue[19] = "pg2.bmp";
    dialogue[20] = "pg3.bmp";
    dialogue[21] = "tourguide17.bmp";
    dialogue[22] = "tourguide18.bmp";
    dialogue[23] = "tourguide19.bmp";
    dialogue[24] = "female1.bmp";
    dialogue[25] = "information0.bmp";
    dialogue[26] = "female2.bmp";
    dialogue[27] = "tourguide20.bmp";
    dialogue[28] = "tourguide21.bmp";
    dialogue[29] = "tourguide22.bmp";
    dialogue[30] = "tourguide23.bmp";
    dialogue[31] = "tourguide24.bmp";
    dialogue[32] = "female3.bmp";
    dialogue[33] = "pg4.bmp";
    dialogue[34] = "information1.bmp";
    dialogue[35] = "pg5.bmp";
    dialogue[36] = "pg6.bmp";
    dialogue[37] = "tourguide25.bmp";
    dialogue[38] = "tourguide26.bmp";
    dialogue[39] = "tourguide27.bmp";
    dialogue[40] = "jc1.bmp";
    dialogue[41] = "jc2.bmp";
    dialogue[42] = "jc3.bmp";
    dialogue[43] = "jc4.bmp";
    dialogue[44] = "jc5.bmp";
    dialogue[45] = "Transition.bmp";
    dialogue[46] = "jc6.bmp";
    dialogue[47] = "jc7.bmp";
    dialogue[48] = "female_engineer1.bmp";
    dialogue[49] = "jc8.bmp";
    dialogue[50] = "jc9.bmp";
    dialogue[51] = "male_guardsman1.bmp";
    dialogue[52] = "jc10.bmp";
    dialogue[53] = "male_np1.bmp";
    dialogue[54] = "male_chemist1.bmp";
    dialogue[55] = "jc11.bmp";
    dialogue[56] = "jc12.bmp";
    dialogue[57] = "jc13.bmp";
    dialogue[58] = "male_np2.bmp";
    dialogue[59] = "jc14.bmp";
    dialogue[60] = "jc15.bmp";
    dialogue[61] = "male_guardsman2.bmp";
    dialogue[62] = "female_engineer2.bmp";
    dialogue[63] = "male_guardsman3.bmp";
    dialogue[64] = "jc16.bmp";
    dialogue[65] = "jc17.bmp";
    dialogue[66] = "jc18.bmp";
    dialogue[67] = "male_chemist2.bmp";
    dialogue[68] = "jc19.bmp";
    dialogue[69] = "jc20.bmp";
    dialogue[70] = "jc21.bmp";
    dialogue[71] = "jc22.bmp";
    dialogue[72] = "jc23.bmp";
    dialogue[73] = "freemode.bmp";
    dialogue[74] = "";
    dialogue[75] = "";
    dialogue[76] = "";
    dialogue[77] = "";
    dialogue[78] = "";

    journal[0] = hbitmap[224];
    journal[1] = hbitmap[225];
    journal[2] = hbitmap[226];
    journal[3] = hbitmap[227];
    journal[4] = hbitmap[228];
    journal[5] = hbitmap[229];
    journal[6] = hbitmap[230];
    
    mouseclick = true;
    dindex = -1;
    current_dialogue = ".//Dialogue//"+dialogue[0];
    current_journal = journal[0];
}

void Core::initialGame() {

	hbitmap[0]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\main_map.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[345]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\main_map_small.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[1]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\HUD\\CharBar1024Item.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[2]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\attack.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[3]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\move.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[4]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\jornal.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[5]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\useitem.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[6]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\useskill.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[7]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\map.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[8]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[9]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\commons.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[10]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\jeb1.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[11]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\jeb2.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[12]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\jeb3.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[13]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[14]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[15]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[16]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[17]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[18]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[19]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[20]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[21]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[22]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Buttons\\status.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[23]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Dialogue\\background.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[24]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Dialogue\\help.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[25]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Dialogue\\stat.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[26]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\Maps\\map.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[27]=(HBITMAP)LoadImage((HINSTANCE)NULL, ".\\HUD\\CharBar1024.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hbitmap[100]=pGraphic->LoadSplashImage(IDB_PNG1);
	hbitmap[101]=pGraphic->LoadSplashImage(IDB_PNG2);
	hbitmap[102]=pGraphic->LoadSplashImage(IDB_PNG3);
	hbitmap[103]=pGraphic->LoadSplashImage(IDB_PNG4);
	hbitmap[104]=pGraphic->LoadSplashImage(IDB_PNG5);
	hbitmap[105]=pGraphic->LoadSplashImage(IDB_PNG6);
	hbitmap[106]=pGraphic->LoadSplashImage(IDB_PNG7);
	hbitmap[107]=pGraphic->LoadSplashImage(IDB_PNG8);
	hbitmap[108]=pGraphic->LoadSplashImage(IDB_PNG9);
	hbitmap[109]=pGraphic->LoadSplashImage(IDB_PNG10);
	hbitmap[110]=pGraphic->LoadSplashImage(IDB_PNG11);
	hbitmap[111]=pGraphic->LoadSplashImage(IDB_PNG12);
	hbitmap[112]=pGraphic->LoadSplashImage(IDB_PNG13);
	hbitmap[113]=pGraphic->LoadSplashImage(IDB_PNG14);
	hbitmap[114]=pGraphic->LoadSplashImage(IDB_PNG15);
	hbitmap[115]=pGraphic->LoadSplashImage(IDB_PNG16);
	hbitmap[116]=pGraphic->LoadSplashImage(IDB_PNG17);
	hbitmap[117]=pGraphic->LoadSplashImage(IDB_PNG18);
	hbitmap[118]=pGraphic->LoadSplashImage(IDB_PNG19);
	hbitmap[119]=pGraphic->LoadSplashImage(IDB_PNG20);
	hbitmap[120]=pGraphic->LoadSplashImage(IDB_PNG21);
	hbitmap[121]=pGraphic->LoadSplashImage(IDB_PNG22);
	hbitmap[122]=pGraphic->LoadSplashImage(IDB_PNG23);
	hbitmap[123]=pGraphic->LoadSplashImage(IDB_PNG24);
	hbitmap[124]=pGraphic->LoadSplashImage(IDB_PNG25);
	hbitmap[125]=pGraphic->LoadSplashImage(IDB_PNG26);
	hbitmap[126]=pGraphic->LoadSplashImage(IDB_PNG27);
	hbitmap[127]=pGraphic->LoadSplashImage(IDB_PNG28);
	hbitmap[128]=pGraphic->LoadSplashImage(IDB_PNG29);
	hbitmap[129]=pGraphic->LoadSplashImage(IDB_PNG30);
	hbitmap[130]=pGraphic->LoadSplashImage(IDB_PNG31);
	hbitmap[131]=pGraphic->LoadSplashImage(IDB_PNG32);
	hbitmap[132]=pGraphic->LoadSplashImage(IDB_PNG33);
	hbitmap[133]=pGraphic->LoadSplashImage(IDB_PNG34);
	hbitmap[134]=pGraphic->LoadSplashImage(IDB_PNG35);
	hbitmap[135]=pGraphic->LoadSplashImage(IDB_PNG36);
	hbitmap[136]=pGraphic->LoadSplashImage(IDB_PNG37);
	hbitmap[137]=pGraphic->LoadSplashImage(IDB_PNG38);
	hbitmap[138]=pGraphic->LoadSplashImage(IDB_PNG39);
	hbitmap[139]=pGraphic->LoadSplashImage(IDB_PNG40);
	hbitmap[140]=pGraphic->LoadSplashImage(IDB_PNG41);
	hbitmap[141]=pGraphic->LoadSplashImage(IDB_PNG42);
	hbitmap[142]=pGraphic->LoadSplashImage(IDB_PNG43);
	hbitmap[143]=pGraphic->LoadSplashImage(IDB_PNG44);
	hbitmap[144]=pGraphic->LoadSplashImage(IDB_PNG45);
	hbitmap[145]=pGraphic->LoadSplashImage(IDB_PNG46);
	hbitmap[146]=pGraphic->LoadSplashImage(IDB_PNG47);
	hbitmap[147]=pGraphic->LoadSplashImage(IDB_PNG48);
	hbitmap[148]=pGraphic->LoadSplashImage(IDB_PNG49);
	hbitmap[149]=pGraphic->LoadSplashImage(IDB_PNG50);
	hbitmap[150]=pGraphic->LoadSplashImage(IDB_PNG51);
	hbitmap[151]=pGraphic->LoadSplashImage(IDB_PNG52);
	hbitmap[152]=pGraphic->LoadSplashImage(IDB_PNG53);
	hbitmap[153]=pGraphic->LoadSplashImage(IDB_PNG54);
	hbitmap[154]=pGraphic->LoadSplashImage(IDB_PNG55);
	hbitmap[155]=pGraphic->LoadSplashImage(IDB_PNG56);
	hbitmap[156]=pGraphic->LoadSplashImage(IDB_PNG57);
	hbitmap[157]=pGraphic->LoadSplashImage(IDB_PNG58);
	hbitmap[158]=pGraphic->LoadSplashImage(IDB_PNG59);
	hbitmap[159]=pGraphic->LoadSplashImage(IDB_PNG60);
	hbitmap[160]=pGraphic->LoadSplashImage(IDB_PNG61);
	hbitmap[161]=pGraphic->LoadSplashImage(IDB_PNG62);
	hbitmap[162]=pGraphic->LoadSplashImage(IDB_PNG63);
	hbitmap[163]=pGraphic->LoadSplashImage(IDB_PNG64);
	hbitmap[164]=pGraphic->LoadSplashImage(IDB_PNG65);
	hbitmap[165]=pGraphic->LoadSplashImage(IDB_PNG66);
	hbitmap[166]=pGraphic->LoadSplashImage(IDB_PNG67);
	hbitmap[167]=pGraphic->LoadSplashImage(IDB_PNG68);
	hbitmap[168]=pGraphic->LoadSplashImage(IDB_PNG69);
	hbitmap[169]=pGraphic->LoadSplashImage(IDB_PNG70);
	hbitmap[170]=pGraphic->LoadSplashImage(IDB_PNG71);
	hbitmap[171]=pGraphic->LoadSplashImage(IDB_PNG72);
	hbitmap[172]=pGraphic->LoadSplashImage(IDB_PNG73);
	hbitmap[173]=pGraphic->LoadSplashImage(IDB_PNG74);
	hbitmap[174]=pGraphic->LoadSplashImage(IDB_PNG75);
	hbitmap[175]=pGraphic->LoadSplashImage(IDB_PNG76);
	hbitmap[176]=pGraphic->LoadSplashImage(IDB_PNG77);
	hbitmap[177]=pGraphic->LoadSplashImage(IDB_PNG78);
	hbitmap[178]=pGraphic->LoadSplashImage(IDB_PNG79);
	hbitmap[179]=pGraphic->LoadSplashImage(IDB_PNG80);
	hbitmap[180]=pGraphic->LoadSplashImage(IDB_PNG81);
	hbitmap[181]=pGraphic->LoadSplashImage(IDB_PNG82);
	hbitmap[182]=pGraphic->LoadSplashImage(IDB_PNG83);
	hbitmap[183]=pGraphic->LoadSplashImage(IDB_PNG84);
	hbitmap[184]=pGraphic->LoadSplashImage(IDB_PNG85);
	hbitmap[185]=pGraphic->LoadSplashImage(IDB_PNG86);
	hbitmap[186]=pGraphic->LoadSplashImage(IDB_PNG87);
	hbitmap[187]=pGraphic->LoadSplashImage(IDB_PNG88);
	hbitmap[188]=pGraphic->LoadSplashImage(IDB_PNG89);
	hbitmap[189]=pGraphic->LoadSplashImage(IDB_PNG90);
	hbitmap[190]=pGraphic->LoadSplashImage(IDB_PNG91);
	hbitmap[191]=pGraphic->LoadSplashImage(IDB_PNG92);
	hbitmap[192]=pGraphic->LoadSplashImage(IDB_PNG93);
	hbitmap[193]=pGraphic->LoadSplashImage(IDB_PNG94);
	hbitmap[194]=pGraphic->LoadSplashImage(IDB_PNG95);
	hbitmap[195]=pGraphic->LoadSplashImage(IDB_PNG96);
	hbitmap[196]=pGraphic->LoadSplashImage(IDB_PNG97);
	hbitmap[197]=pGraphic->LoadSplashImage(IDB_PNG98);
	hbitmap[198]=pGraphic->LoadSplashImage(IDB_PNG99);
	hbitmap[199]=pGraphic->LoadSplashImage(IDB_PNG100);
	hbitmap[200]=pGraphic->LoadSplashImage(IDB_PNG101);
	hbitmap[201]=pGraphic->LoadSplashImage(IDB_PNG102);
	hbitmap[202]=pGraphic->LoadSplashImage(IDB_PNG103);
	hbitmap[203]=pGraphic->LoadSplashImage(IDB_PNG104);
	hbitmap[204]=pGraphic->LoadSplashImage(IDB_PNG105);
	hbitmap[205]=pGraphic->LoadSplashImage(IDB_PNG106);
	hbitmap[206]=pGraphic->LoadSplashImage(IDB_PNG107);
	hbitmap[207]=pGraphic->LoadSplashImage(IDB_PNG108);
	hbitmap[208]=pGraphic->LoadSplashImage(IDB_PNG109);
	hbitmap[209]=pGraphic->LoadSplashImage(IDB_PNG110);
	hbitmap[210]=pGraphic->LoadSplashImage(IDB_PNG111);
	hbitmap[211]=pGraphic->LoadSplashImage(IDB_PNG112);
	hbitmap[212]=pGraphic->LoadSplashImage(IDB_PNG113);
	hbitmap[213]=pGraphic->LoadSplashImage(IDB_PNG114);
	hbitmap[214]=pGraphic->LoadSplashImage(IDB_PNG115);
	hbitmap[215]=pGraphic->LoadSplashImage(IDB_PNG116);
	hbitmap[216]=pGraphic->LoadSplashImage(IDB_PNG117);
	hbitmap[217]=pGraphic->LoadSplashImage(IDB_PNG118);
	hbitmap[218]=pGraphic->LoadSplashImage(IDB_PNG119);
	hbitmap[219]=pGraphic->LoadSplashImage(IDB_PNG120);
	hbitmap[220]=pGraphic->LoadSplashImage(IDB_PNG121);
	hbitmap[221]=pGraphic->LoadSplashImage(IDB_PNG122);
	hbitmap[222]=pGraphic->LoadSplashImage(IDB_PNG123);
	hbitmap[223]=pGraphic->LoadSplashImage(IDB_PNG124);
	hbitmap[224]=pGraphic->LoadSplashImage(IDB_PNG125);
	hbitmap[225]=pGraphic->LoadSplashImage(IDB_PNG126);
	hbitmap[226]=pGraphic->LoadSplashImage(IDB_PNG127);
	hbitmap[227]=pGraphic->LoadSplashImage(IDB_PNG128);
	hbitmap[228]=pGraphic->LoadSplashImage(IDB_PNG129);
	hbitmap[229]=pGraphic->LoadSplashImage(IDB_PNG130);
	hbitmap[230]=pGraphic->LoadSplashImage(IDB_PNG131);
	hbitmap[231]=pGraphic->LoadSplashImage(IDB_PNG132);
	hbitmap[232]=pGraphic->LoadSplashImage(IDB_PNG133);

	hbitmap[233]=pGraphic->LoadSplashImage(IDB_PNG134);
	hbitmap[234]=pGraphic->LoadSplashImage(IDB_PNG135);
	hbitmap[235]=pGraphic->LoadSplashImage(IDB_PNG136);
	hbitmap[236]=pGraphic->LoadSplashImage(IDB_PNG137);
	hbitmap[237]=pGraphic->LoadSplashImage(IDB_PNG138);
	hbitmap[238]=pGraphic->LoadSplashImage(IDB_PNG139);
	hbitmap[239]=pGraphic->LoadSplashImage(IDB_PNG140);
	hbitmap[240]=pGraphic->LoadSplashImage(IDB_PNG141);
	hbitmap[241]=pGraphic->LoadSplashImage(IDB_PNG142);
	hbitmap[242]=pGraphic->LoadSplashImage(IDB_PNG143);
	hbitmap[243]=pGraphic->LoadSplashImage(IDB_PNG144);
	hbitmap[244]=pGraphic->LoadSplashImage(IDB_PNG145);

	hbitmap[245]=pGraphic->LoadSplashImage(IDB_PNG146);
	hbitmap[246]=pGraphic->LoadSplashImage(IDB_PNG147);
	hbitmap[247]=pGraphic->LoadSplashImage(IDB_PNG148);
	hbitmap[248]=pGraphic->LoadSplashImage(IDB_PNG149);
	hbitmap[249]=pGraphic->LoadSplashImage(IDB_PNG150);
	hbitmap[250]=pGraphic->LoadSplashImage(IDB_PNG151);
	hbitmap[251]=pGraphic->LoadSplashImage(IDB_PNG152);
	hbitmap[252]=pGraphic->LoadSplashImage(IDB_PNG153);
	hbitmap[253]=pGraphic->LoadSplashImage(IDB_PNG154);
	hbitmap[254]=pGraphic->LoadSplashImage(IDB_PNG155);
	hbitmap[255]=pGraphic->LoadSplashImage(IDB_PNG156);
	hbitmap[256]=pGraphic->LoadSplashImage(IDB_PNG157);

	hbitmap[257]=pGraphic->LoadSplashImage(IDB_PNG158);
	hbitmap[258]=pGraphic->LoadSplashImage(IDB_PNG159);
	hbitmap[259]=pGraphic->LoadSplashImage(IDB_PNG160);
	hbitmap[260]=pGraphic->LoadSplashImage(IDB_PNG161);
	hbitmap[261]=pGraphic->LoadSplashImage(IDB_PNG162);
	hbitmap[262]=pGraphic->LoadSplashImage(IDB_PNG163);
	hbitmap[263]=pGraphic->LoadSplashImage(IDB_PNG164);
	hbitmap[264]=pGraphic->LoadSplashImage(IDB_PNG165);
	hbitmap[265]=pGraphic->LoadSplashImage(IDB_PNG166);
	hbitmap[266]=pGraphic->LoadSplashImage(IDB_PNG167);
	hbitmap[267]=pGraphic->LoadSplashImage(IDB_PNG168);
	hbitmap[268]=pGraphic->LoadSplashImage(IDB_PNG169);
	//transparent shade blue square for debug
	hbitmap[269] = pGraphic->LoadSplashImage(IDB_PNG170);

	hbitmap[270]=pGraphic->LoadSplashImage(IDB_PNG171);
	hbitmap[271]=pGraphic->LoadSplashImage(IDB_PNG172);
	hbitmap[272]=pGraphic->LoadSplashImage(IDB_PNG173);
	hbitmap[273]=pGraphic->LoadSplashImage(IDB_PNG174);
	hbitmap[274]=pGraphic->LoadSplashImage(IDB_PNG175);
	hbitmap[275]=pGraphic->LoadSplashImage(IDB_PNG176);
	hbitmap[276]=pGraphic->LoadSplashImage(IDB_PNG177);
	hbitmap[277]=pGraphic->LoadSplashImage(IDB_PNG178);
	hbitmap[278]=pGraphic->LoadSplashImage(IDB_PNG179);
	hbitmap[279]=pGraphic->LoadSplashImage(IDB_PNG180);
	hbitmap[280]=pGraphic->LoadSplashImage(IDB_PNG181);
	hbitmap[281]=pGraphic->LoadSplashImage(IDB_PNG182);

	hbitmap[282]=pGraphic->LoadSplashImage(IDB_PNG183);
	hbitmap[283]=pGraphic->LoadSplashImage(IDB_PNG184);
	hbitmap[284]=pGraphic->LoadSplashImage(IDB_PNG185);
	hbitmap[285]=pGraphic->LoadSplashImage(IDB_PNG186);
	hbitmap[286]=pGraphic->LoadSplashImage(IDB_PNG187);
	hbitmap[287]=pGraphic->LoadSplashImage(IDB_PNG188);
	hbitmap[288]=pGraphic->LoadSplashImage(IDB_PNG189);
	hbitmap[289]=pGraphic->LoadSplashImage(IDB_PNG190);
	hbitmap[290]=pGraphic->LoadSplashImage(IDB_PNG191);
	hbitmap[291]=pGraphic->LoadSplashImage(IDB_PNG192);
	hbitmap[292]=pGraphic->LoadSplashImage(IDB_PNG193);
	hbitmap[293]=pGraphic->LoadSplashImage(IDB_PNG194);

	hbitmap[294]=pGraphic->LoadSplashImage(IDB_PNG195);
	hbitmap[295]=pGraphic->LoadSplashImage(IDB_PNG196);
	hbitmap[296]=pGraphic->LoadSplashImage(IDB_PNG197);
	hbitmap[297]=pGraphic->LoadSplashImage(IDB_PNG198);
	hbitmap[298]=pGraphic->LoadSplashImage(IDB_PNG199);
	hbitmap[299]=pGraphic->LoadSplashImage(IDB_PNG200);
	hbitmap[300]=pGraphic->LoadSplashImage(IDB_PNG201);
	hbitmap[301]=pGraphic->LoadSplashImage(IDB_PNG202);
	hbitmap[302]=pGraphic->LoadSplashImage(IDB_PNG203);
	hbitmap[303]=pGraphic->LoadSplashImage(IDB_PNG204);
	hbitmap[304]=pGraphic->LoadSplashImage(IDB_PNG205);
	hbitmap[305]=pGraphic->LoadSplashImage(IDB_PNG206);

	hbitmap[306]=pGraphic->LoadSplashImage(IDB_PNG207);
	hbitmap[307]=pGraphic->LoadSplashImage(IDB_PNG208);
	hbitmap[308]=pGraphic->LoadSplashImage(IDB_PNG209);
	hbitmap[309]=pGraphic->LoadSplashImage(IDB_PNG210);
	hbitmap[310]=pGraphic->LoadSplashImage(IDB_PNG211);
	hbitmap[311]=pGraphic->LoadSplashImage(IDB_PNG212);
	hbitmap[312]=pGraphic->LoadSplashImage(IDB_PNG213);
	hbitmap[313]=pGraphic->LoadSplashImage(IDB_PNG214);
	hbitmap[314]=pGraphic->LoadSplashImage(IDB_PNG215);
	hbitmap[315]=pGraphic->LoadSplashImage(IDB_PNG216);
	hbitmap[316]=pGraphic->LoadSplashImage(IDB_PNG217);
	hbitmap[317]=pGraphic->LoadSplashImage(IDB_PNG218);

	hbitmap[318]=pGraphic->LoadSplashImage(IDB_PNG219);
	hbitmap[319]=pGraphic->LoadSplashImage(IDB_PNG220);
	hbitmap[320]=pGraphic->LoadSplashImage(IDB_PNG221);
	hbitmap[321]=pGraphic->LoadSplashImage(IDB_PNG222);
	hbitmap[322]=pGraphic->LoadSplashImage(IDB_PNG223);
	hbitmap[323]=pGraphic->LoadSplashImage(IDB_PNG224);
	hbitmap[324]=pGraphic->LoadSplashImage(IDB_PNG225);
	hbitmap[325]=pGraphic->LoadSplashImage(IDB_PNG226);
	hbitmap[326]=pGraphic->LoadSplashImage(IDB_PNG227);
	hbitmap[327]=pGraphic->LoadSplashImage(IDB_PNG228);
	hbitmap[328]=pGraphic->LoadSplashImage(IDB_PNG229);
	hbitmap[329]=pGraphic->LoadSplashImage(IDB_PNG230);

	hbitmap[330]=pGraphic->LoadSplashImage(IDB_PNG231);
	hbitmap[331]=pGraphic->LoadSplashImage(IDB_PNG232);
	hbitmap[332]=pGraphic->LoadSplashImage(IDB_PNG233);
	hbitmap[333]=pGraphic->LoadSplashImage(IDB_PNG234);
	hbitmap[334]=pGraphic->LoadSplashImage(IDB_PNG235);
	hbitmap[335]=pGraphic->LoadSplashImage(IDB_PNG236);
	hbitmap[336]=pGraphic->LoadSplashImage(IDB_PNG237);
	hbitmap[337]=pGraphic->LoadSplashImage(IDB_PNG238);
	hbitmap[338]=pGraphic->LoadSplashImage(IDB_PNG239);
	hbitmap[339]=pGraphic->LoadSplashImage(IDB_PNG240);
	hbitmap[340]=pGraphic->LoadSplashImage(IDB_PNG241);
	hbitmap[341]=pGraphic->LoadSplashImage(IDB_PNG242);

	//mouse ripple
	hbitmap[342]=pGraphic->LoadSplashImage(IDB_PNG243);
	hbitmap[343]=pGraphic->LoadSplashImage(IDB_PNG244);
	hbitmap[344]=pGraphic->LoadSplashImage(IDB_PNG245);



	jornalclicked=false;
	mapclicked=false;
	statusclicked=false;
	singlestate = 2;
	f1pressed = false;
	f2pressed = false;
	f3pressed = false;
	f4pressed = false;
	f5pressed = false;
	f6pressed = false;
	f7pressed = false;
	globalBGMVolume = 0.3f;
	currentMonitoredNPC = 4;
	gameTimer.Reset();
	BGMVolDispTimer.Reset();
	BGMVolDispSwitch = false;
 	currentPlayer = 0;
	srand (static_cast<unsigned int>(time((time_t)NULL)));
	mode=0;//starting animation: background introduction
	dialogue_initial(); 
	antiAround = { false, 0 };
	//sound_api = XAUDIO2_API;
	sound_api = OPENAL_API;
	pSoundDevice = new SoundDevice(sound_api);
	noticequeue = new NoticeQueue(pSoundDevice);
	teampop = new TeamPop(pSoundDevice, singlestate);
  string namepool[]={"Lewis K. Mack",
		     "Julius Petrel Gilligan Jr.",
		     "H. Drassilis Gerenraip",
		     "ElishaEmmett Conneeny Gregson",
		     "E. W. Cosgriff III",
		     "Michael Sy Naghton Jr.",
		     "Buck I. Hardyng III",
		     "Donald G. Whitehill IV",
		     "Gabriel M. Velvetscoter III",
		     "Simeon Earle Fry",
		     "C. Campion Brown Jr.",
		     "Abraham A. Deegan",
		     "Leonard McNamee Hoskyns Jr.",
		     "B. Camin McAnnually",
		     "Levi B. Proud",
		     "G. Freyn Reamsbottom",
		     "Clemence Grinan Proctor Jr.",
		     "Victor Hussey Wotton",
		     "Ora Hefernan Macken",
		     "Alexander T. Callan",
		     "Jeptha Brookes Reddan Jr.",
		     "Arthur Brickleband Remington IV",
		     "Bingley H. Forbes Jr.",
		     "H. Meyer Lancey Jr.",
		     "Jeptha Carden Hayton",
		     "G. R. Jenkins",
		     "I. Nichols Gough",
		     "Elijah Hoban Hickey Jr.",
		     "Lucian Hank Velvetscoter",
		     "Perry T. Egars"
  };


  player_female_active[0][0]=hbitmap[100];
  player_female_active[1][0]=hbitmap[101];
  player_female_active[2][0]=hbitmap[102];
  player_female_active[3][0]=hbitmap[103];

  player_female_dead[0][0]=hbitmap[104];
  player_female_dead[1][0]=hbitmap[105];
  player_female_dead[2][0]=hbitmap[106];
  player_female_dead[3][0]=hbitmap[107];

  player_female_deading[0][0]=hbitmap[108];
  player_female_deading[0][1]=hbitmap[109];
  player_female_deading[1][0]=hbitmap[110];
  player_female_deading[1][1]=hbitmap[111];
  player_female_deading[2][0]=hbitmap[112];
  player_female_deading[2][1]=hbitmap[113];
  player_female_deading[3][0]=hbitmap[114];
  player_female_deading[3][1]=hbitmap[115];

  player_female_grenade[0][0]=hbitmap[116];
  player_female_grenade[0][1]=hbitmap[117];
  player_female_grenade[1][0]=hbitmap[118];
  player_female_grenade[1][1]=hbitmap[119];
  player_female_grenade[2][0]=hbitmap[120];
  player_female_grenade[2][1]=hbitmap[121];
  player_female_grenade[3][0]=hbitmap[122];
  player_female_grenade[3][1]=hbitmap[123];

  player_female_hit[0][0]=hbitmap[124];
  player_female_hit[0][1]=hbitmap[125];
  player_female_hit[1][0]=hbitmap[126];
  player_female_hit[1][1]=hbitmap[127];
  player_female_hit[2][0]=hbitmap[128];
  player_female_hit[2][1]=hbitmap[129];
  player_female_hit[3][0]=hbitmap[130];
  player_female_hit[3][1]=hbitmap[131];

  player_female_stopping[0][0]=hbitmap[132];
  player_female_stopping[1][0]=hbitmap[133];
  player_female_stopping[2][0]=hbitmap[134];
  player_female_stopping[3][0]=hbitmap[135];

  player_female_magic[0][0]=hbitmap[136];
  player_female_magic[1][0]=hbitmap[137];
  player_female_magic[2][0]=hbitmap[138];
  player_female_magic[3][0]=hbitmap[139];

  player_female_pistol[0][0]=hbitmap[140];
  player_female_pistol[0][1]=hbitmap[141];
  player_female_pistol[0][2]=hbitmap[142];
  player_female_pistol[1][0]=hbitmap[143];
  player_female_pistol[1][1]=hbitmap[144];
  player_female_pistol[1][2]=hbitmap[145];
  player_female_pistol[2][0]=hbitmap[146];
  player_female_pistol[2][1]=hbitmap[147];
  player_female_pistol[2][2]=hbitmap[148];
  player_female_pistol[3][0]=hbitmap[149];
  player_female_pistol[3][1]=hbitmap[150];
  player_female_pistol[3][2]=hbitmap[151];

  player_female_attacking[0][0]=hbitmap[152];
  player_female_attacking[0][1]=hbitmap[153];
  player_female_attacking[1][0]=hbitmap[154];
  player_female_attacking[1][1]=hbitmap[155];
  player_female_attacking[2][0]=hbitmap[156];
  player_female_attacking[2][1]=hbitmap[157];
  player_female_attacking[3][0]=hbitmap[158];
  player_female_attacking[3][1]=hbitmap[159];

  player_female_moving[0][0]=hbitmap[160];
  player_female_moving[0][1]=hbitmap[161];
  player_female_moving[0][2]=hbitmap[162];
  player_female_moving[1][0]=hbitmap[163];
  player_female_moving[1][1]=hbitmap[164];
  player_female_moving[1][2]=hbitmap[165];
  player_female_moving[2][0]=hbitmap[166];
  player_female_moving[2][1]=hbitmap[167];
  player_female_moving[2][2]=hbitmap[168];
  player_female_moving[3][0]=hbitmap[169];
  player_female_moving[3][1]=hbitmap[170];
  player_female_moving[3][2]=hbitmap[171];



  player_male_active[0][0]=hbitmap[270];
  player_male_active[1][0]=hbitmap[271];
  player_male_active[2][0]=hbitmap[272];
  player_male_active[3][0]=hbitmap[273];

  player_male_dead[0][0]=hbitmap[274];
  player_male_dead[1][0]=hbitmap[275];
  player_male_dead[2][0]=hbitmap[276];
  player_male_dead[3][0]=hbitmap[277];

  player_male_deading[0][0]=hbitmap[278];
  player_male_deading[0][1]=hbitmap[279];
  player_male_deading[1][0]=hbitmap[280];
  player_male_deading[1][1]=hbitmap[281];
  player_male_deading[2][0]=hbitmap[282];
  player_male_deading[2][1]=hbitmap[283];
  player_male_deading[3][0]=hbitmap[284];
  player_male_deading[3][1]=hbitmap[285];

  player_male_grenade[0][0]=hbitmap[286];
  player_male_grenade[0][1]=hbitmap[287];
  player_male_grenade[1][0]=hbitmap[288];
  player_male_grenade[1][1]=hbitmap[289];
  player_male_grenade[2][0]=hbitmap[290];
  player_male_grenade[2][1]=hbitmap[291];
  player_male_grenade[3][0]=hbitmap[292];
  player_male_grenade[3][1]=hbitmap[293];

  player_male_hit[0][0]=hbitmap[294];
  player_male_hit[0][1]=hbitmap[295];
  player_male_hit[1][0]=hbitmap[296];
  player_male_hit[1][1]=hbitmap[297];
  player_male_hit[2][0]=hbitmap[298];
  player_male_hit[2][1]=hbitmap[299];
  player_male_hit[3][0]=hbitmap[300];
  player_male_hit[3][1]=hbitmap[301];

  player_male_stopping[0][0]=hbitmap[302];
  player_male_stopping[1][0]=hbitmap[303];
  player_male_stopping[2][0]=hbitmap[304];
  player_male_stopping[3][0]=hbitmap[305];

  player_male_magic[0][0]=hbitmap[306];
  player_male_magic[1][0]=hbitmap[307];
  player_male_magic[2][0]=hbitmap[308];
  player_male_magic[3][0]=hbitmap[309];

  player_male_pistol[0][0]=hbitmap[310];
  player_male_pistol[0][1]=hbitmap[311];
  player_male_pistol[0][2]=hbitmap[312];
  player_male_pistol[1][0]=hbitmap[313];
  player_male_pistol[1][1]=hbitmap[314];
  player_male_pistol[1][2]=hbitmap[315];
  player_male_pistol[2][0]=hbitmap[316];
  player_male_pistol[2][1]=hbitmap[317];
  player_male_pistol[2][2]=hbitmap[318];
  player_male_pistol[3][0]=hbitmap[319];
  player_male_pistol[3][1]=hbitmap[320];
  player_male_pistol[3][2]=hbitmap[321];

  player_male_attacking[0][0]=hbitmap[322];
  player_male_attacking[0][1]=hbitmap[323];
  player_male_attacking[1][0]=hbitmap[324];
  player_male_attacking[1][1]=hbitmap[325];
  player_male_attacking[2][0]=hbitmap[326];
  player_male_attacking[2][1]=hbitmap[327];
  player_male_attacking[3][0]=hbitmap[328];
  player_male_attacking[3][1]=hbitmap[329];

  player_male_moving[0][0]=hbitmap[330];
  player_male_moving[0][1]=hbitmap[331];
  player_male_moving[0][2]=hbitmap[332];
  player_male_moving[1][0]=hbitmap[333];
  player_male_moving[1][1]=hbitmap[334];
  player_male_moving[1][2]=hbitmap[335];
  player_male_moving[2][0]=hbitmap[336];
  player_male_moving[2][1]=hbitmap[337];
  player_male_moving[2][2]=hbitmap[338];
  player_male_moving[3][0]=hbitmap[339];
  player_male_moving[3][1]=hbitmap[340];
  player_male_moving[3][2]=hbitmap[341];



  mob_attacking[0][0]=hbitmap[172];
  mob_attacking[0][1]=hbitmap[173];
  mob_attacking[1][0]=hbitmap[174];
  mob_attacking[1][1]=hbitmap[175];
  mob_attacking[2][0]=hbitmap[176];
  mob_attacking[2][1]=hbitmap[177];
  mob_attacking[3][0]=hbitmap[178];
  mob_attacking[3][1]=hbitmap[179];

  mob_dead[0][0]=hbitmap[180];
  mob_dead[1][0]=hbitmap[181];
  mob_dead[2][0]=hbitmap[182];
  mob_dead[3][0]=hbitmap[183];

  mob_deading[0][0]=hbitmap[184];
  mob_deading[0][1]=hbitmap[185];
  mob_deading[1][0]=hbitmap[186];
  mob_deading[1][1]=hbitmap[187];
  mob_deading[2][0]=hbitmap[188];
  mob_deading[2][1]=hbitmap[189];
  mob_deading[3][0]=hbitmap[190];
  mob_deading[3][1]=hbitmap[191];

  mob_hit[0][0]=hbitmap[192];
  mob_hit[0][1]=hbitmap[193];
  mob_hit[1][0]=hbitmap[194];
  mob_hit[1][1]=hbitmap[195];
  mob_hit[2][0]=hbitmap[196];
  mob_hit[2][1]=hbitmap[197];
  mob_hit[3][0]=hbitmap[198];
  mob_hit[3][1]=hbitmap[199];

  mob_stopping[0][0]=hbitmap[200];
  mob_stopping[1][0]=hbitmap[201];
  mob_stopping[2][0]=hbitmap[202];
  mob_stopping[3][0]=hbitmap[203];

  mob_moving[0][0]=hbitmap[204];
  mob_moving[0][1]=mob_stopping[0][0];
  mob_moving[0][2]=hbitmap[205];
  mob_moving[1][0]=hbitmap[206];
  mob_moving[1][1]=mob_stopping[1][0];
  mob_moving[1][2]=hbitmap[207];
  mob_moving[2][0]=hbitmap[208];
  mob_moving[2][1]=mob_stopping[2][0];
  mob_moving[2][2]=hbitmap[209];
  mob_moving[3][0]=hbitmap[210];
  mob_moving[3][1]=mob_stopping[3][0];
  mob_moving[3][2]=hbitmap[211];

  goggles_stopping[0][0]=hbitmap[212];
  goggles_stopping[1][0]=hbitmap[213];
  goggles_stopping[2][0]=hbitmap[214];
  goggles_stopping[3][0]=hbitmap[215];

  goggles_moving[0][0]=hbitmap[233];
  goggles_moving[0][1]=hbitmap[234];
  goggles_moving[0][2]=hbitmap[235];
  goggles_moving[1][0]=hbitmap[236];
  goggles_moving[1][1]=hbitmap[237];
  goggles_moving[1][2]=hbitmap[238];
  goggles_moving[2][0]=hbitmap[239];
  goggles_moving[2][1]=hbitmap[240];
  goggles_moving[2][2]=hbitmap[241];
  goggles_moving[3][0]=hbitmap[242];
  goggles_moving[3][1]=hbitmap[243];
  goggles_moving[3][2]=hbitmap[244];

  police_stopping[0][0]=hbitmap[216];
  police_stopping[1][0]=hbitmap[217];
  police_stopping[2][0]=hbitmap[218];
  police_stopping[3][0]=hbitmap[219];

  police_moving[0][0]=hbitmap[245];
  police_moving[0][1]=hbitmap[246];
  police_moving[0][2]=hbitmap[247];
  police_moving[1][0]=hbitmap[248];
  police_moving[1][1]=hbitmap[249];
  police_moving[1][2]=hbitmap[250];
  police_moving[2][0]=hbitmap[251];
  police_moving[2][1]=hbitmap[252];
  police_moving[2][2]=hbitmap[253];
  police_moving[3][0]=hbitmap[254];
  police_moving[3][1]=hbitmap[255];
  police_moving[3][2]=hbitmap[256];

  jint_stopping[0][0]=hbitmap[220];
  jint_stopping[1][0]=hbitmap[221];
  jint_stopping[2][0]=hbitmap[222];
  jint_stopping[3][0]=hbitmap[223];

  jint_moving[0][0]=hbitmap[257];
  jint_moving[0][1]=hbitmap[258];
  jint_moving[0][2]=hbitmap[259];
  jint_moving[1][0]=hbitmap[260];
  jint_moving[1][1]=hbitmap[261];
  jint_moving[1][2]=hbitmap[262];
  jint_moving[2][0]=hbitmap[263];
  jint_moving[2][1]=hbitmap[264];
  jint_moving[2][2]=hbitmap[265];
  jint_moving[3][0]=hbitmap[266];
  jint_moving[3][1]=hbitmap[267];
  jint_moving[3][2]=hbitmap[268];

  mouse_ripple[0][0] = hbitmap[344];
  mouse_ripple[0][1] = hbitmap[343];
  mouse_ripple[0][2] = hbitmap[342];


  mouseripple = new Animation(mouse_ripple, 1, 3, 0.2f);

  //for readability, here use STL hashmap and STL string as the key, 
  //for real use or specific memory and speed demand, it could be changed to dynamic array using int as accessing key. And use all char* instead of STL string class
  unordered_map<string, Sound*>* main_map = new unordered_map<string, Sound*>({
	  {"shoe1", new Sound(pSoundDevice,".//Sound//Effects//shoe1_main.wav", 0, 2)},
	  {"shoe2", new Sound(pSoundDevice,".//Sound//Effects//shoe2_main.wav", 0, 2)},
	  {"shoe3", new Sound(pSoundDevice,".//Sound//Effects//shoe3_main.wav", 0, 2)},
	  {"shoe4", new Sound(pSoundDevice,".//Sound//Effects//shoe4_main.wav", 0, 2)},
	  {"shoe5", new Sound(pSoundDevice,".//Sound//Effects//shoe5_main.wav", 0, 2)},
	  {"shoe6", new Sound(pSoundDevice,".//Sound//Effects//shoe6_main.wav", 0, 2)},
	  {"shoe7", new Sound(pSoundDevice,".//Sound//Effects//shoe7_main.wav", 0, 2)},
	  {"shoe8", new Sound(pSoundDevice,".//Sound//Effects//shoe8_main.wav", 0, 2)} 
  });
  unordered_map<string, Sound*>* jeb1 = new unordered_map<string, Sound*>({
	  {"shoe1", new Sound(pSoundDevice,".//Sound//Effects//shoe1_jeb1.wav", 0, 2)},
	  {"shoe2", new Sound(pSoundDevice,".//Sound//Effects//shoe2_jeb1.wav", 0, 2)},
	  {"shoe3", new Sound(pSoundDevice,".//Sound//Effects//shoe3_jeb1.wav", 0, 2)},
	  {"shoe4", new Sound(pSoundDevice,".//Sound//Effects//shoe4_jeb1.wav", 0, 2)},
	  {"shoe5", new Sound(pSoundDevice,".//Sound//Effects//shoe5_jeb1.wav", 0, 2)},
	  {"shoe6", new Sound(pSoundDevice,".//Sound//Effects//shoe6_jeb1.wav", 0, 2)},
	  {"shoe7", new Sound(pSoundDevice,".//Sound//Effects//shoe7_jeb1.wav", 0, 2)},
	  {"shoe8", new Sound(pSoundDevice,".//Sound//Effects//shoe8_jeb1.wav", 0, 2)} 
  });
  unordered_map<string, Sound*>* jeb2 = new unordered_map<string, Sound*>({
	  {"shoe1", new Sound(pSoundDevice,".//Sound//Effects//shoe1_jeb2.wav", 0, 2)},
	  {"shoe2", new Sound(pSoundDevice,".//Sound//Effects//shoe2_jeb2.wav", 0, 2)},
	  {"shoe3", new Sound(pSoundDevice,".//Sound//Effects//shoe3_jeb2.wav", 0, 2)},
	  {"shoe4", new Sound(pSoundDevice,".//Sound//Effects//shoe4_jeb2.wav", 0, 2)},
	  {"shoe5", new Sound(pSoundDevice,".//Sound//Effects//shoe5_jeb2.wav", 0, 2)},
	  {"shoe6", new Sound(pSoundDevice,".//Sound//Effects//shoe6_jeb2.wav", 0, 2)},
	  {"shoe7", new Sound(pSoundDevice,".//Sound//Effects//shoe7_jeb2.wav", 0, 2)},
	  {"shoe8", new Sound(pSoundDevice,".//Sound//Effects//shoe8_jeb2.wav", 0, 2)} 
  });
  unordered_map<string, Sound*>* jeb3 = new unordered_map<string, Sound*>({
	  {"shoe1", new Sound(pSoundDevice,".//Sound//Effects//shoe1_jeb3.wav", 0, 2)},
	  {"shoe2", new Sound(pSoundDevice,".//Sound//Effects//shoe2_jeb3.wav", 0, 2)},
	  {"shoe3", new Sound(pSoundDevice,".//Sound//Effects//shoe3_jeb3.wav", 0, 2)},
	  {"shoe4", new Sound(pSoundDevice,".//Sound//Effects//shoe4_jeb3.wav", 0, 2)},
	  {"shoe5", new Sound(pSoundDevice,".//Sound//Effects//shoe5_jeb3.wav", 0, 2)},
	  {"shoe6", new Sound(pSoundDevice,".//Sound//Effects//shoe6_jeb3.wav", 0, 2)},
	  {"shoe7", new Sound(pSoundDevice,".//Sound//Effects//shoe7_jeb3.wav", 0, 2)},
	  {"shoe8", new Sound(pSoundDevice,".//Sound//Effects//shoe8_jeb3.wav", 0, 2)} 
  });
  unordered_map<string, Sound*>* commons = new unordered_map<string, Sound*>({
	  {"shoe1", new Sound(pSoundDevice,".//Sound//Effects//shoe1_commons.wav", 0, 2)},
	  {"shoe2", new Sound(pSoundDevice,".//Sound//Effects//shoe2_commons.wav", 0, 2)},
	  {"shoe3", new Sound(pSoundDevice,".//Sound//Effects//shoe3_commons.wav", 0, 2)},
	  {"shoe4", new Sound(pSoundDevice,".//Sound//Effects//shoe4_commons.wav", 0, 2)},
	  {"shoe5", new Sound(pSoundDevice,".//Sound//Effects//shoe5_commons.wav", 0, 2)},
	  {"shoe6", new Sound(pSoundDevice,".//Sound//Effects//shoe6_commons.wav", 0, 2)},
	  {"shoe7", new Sound(pSoundDevice,".//Sound//Effects//shoe7_commons.wav", 0, 2)},
	  {"shoe8", new Sound(pSoundDevice,".//Sound//Effects//shoe8_commons.wav", 0, 2)} 
  });
  
  vector<MAPSTRUCT> allmaps = {
	  MAPSTRUCT(0, "main_map", 120, 12, 38, 105,".//ASCII//main_map.txt",pSoundDevice,".//Sound//BGM//LostOneSockYesterday.wav", main_map, globalBGMVolume),
	  MAPSTRUCT(1, "jeb1", 40, 4, 18, 55,".//ASCII//jeb1.txt", pSoundDevice, ".//Sound//BGM//SubscriptiontoSuicide.wav", jeb1, globalBGMVolume),
	  MAPSTRUCT(2, "jeb2", 40, 4, 18, 55,".//ASCII//jeb2.txt", pSoundDevice,".//Sound//BGM//VaudevilleDrinkingGame.wav", jeb2, globalBGMVolume),
	  MAPSTRUCT(3, "jeb3", 40, 4, 18, 55,".//ASCII//jeb3.txt", pSoundDevice,".//Sound//BGM//CaffeineCrash.wav", jeb3, globalBGMVolume),
	  MAPSTRUCT(4, "commons", 40, 4, 45, 50,".//ASCII//commons.txt", pSoundDevice,".//Sound//BGM//NewFrontierForgotthe.wav", commons, globalBGMVolume)
  };
  /*
  Sound* main_jeb1 = new Sound(pSoundDevice, ".//Sound//Effects//main_jeb1.wav", 0);
  Sound* common_main = new Sound(pSoundDevice, ".//Sound//Effects//common_main.wav", 0);
  Sound* jeb1_jeb2_bottom = new Sound(pSoundDevice, ".//Sound//Effects//jeb1_jeb2_bottom.wav", 0);
  Sound* jeb1_jeb2_left = new Sound(pSoundDevice, ".//Sound//Effects//jeb1_jeb2_left.wav", 0);
  Sound* jeb2_jeb3_bottom = new Sound(pSoundDevice, ".//Sound//Effects//jeb2_jeb3_bottom.wav", 0);
  Sound* jeb2_jeb3_left = new Sound(pSoundDevice, ".//Sound//Effects//jeb2_jeb3_left.wav", 0);
  */
  vector<pair<pair<string, door*>, pair<string, door*>>> doorpairs = {
	  make_pair(make_pair("main_map", new door(3400,4100,280,2070,320,2160, pSoundDevice, ".//Sound//Effects//main_jeb1.wav")), make_pair("jeb1", new door(320,1960,3580,3700,3780,4000, pSoundDevice, ".//Sound//Effects//main_jeb1.wav"))),
	  make_pair(make_pair("main_map",new door(1560,9960,1735,480,1800,650, pSoundDevice, ".//Sound//Effects//common_main.wav")), make_pair("commons", new door(1680,520,1480,10033,1750,10200, pSoundDevice, ".//Sound//Effects//common_main.wav"))),
	  make_pair(make_pair("jeb1",new door(340,2000,280,2160,320,2200, pSoundDevice, ".//Sound//Effects//jeb1_jeb2_bottom.wav")), make_pair("jeb2", new door(360,2040,360,2070,400,2160, pSoundDevice, ".//Sound//Effects//jeb1_jeb2_bottom.wav"))),//jeb1 bottom to jeb2 bottom
	  make_pair(make_pair("jeb1",new door(200,600,54,480,127,560, pSoundDevice, ".//Sound//Effects//jeb1_jeb2_left.wav")), make_pair("jeb2", new door(200, 560,50,640,140,720, pSoundDevice, ".//Sound//Effects//jeb1_jeb2_left.wav"))), //jeb1 left to jeb2 left
	  make_pair(make_pair("jeb2",new door(360,2040,280,2000,320,2150, pSoundDevice, ".//Sound//Effects//jeb2_jeb3_bottom.wav")), make_pair("jeb3", new door(340,2000,360,2160,400,2200, pSoundDevice, ".//Sound//Effects//jeb2_jeb3_bottom.wav"))),//jeb2 bottom to jeb3 bottom
	  make_pair(make_pair("jeb2",new door(200,560,80,480,140,555, pSoundDevice, ".//Sound//Effects//jeb2_jeb3_left.wav")), make_pair("jeb3", new door(200,600,54,625,127,700, pSoundDevice, ".//Sound//Effects//jeb2_jeb3_left.wav")))//jeb2 left to jeb3 left
	  /*
	  make_pair(make_pair("main_map", new door(3600,3600,280,2070,320,2160, main_jeb1)), make_pair("jeb1", new door(320,1960,3580,3700,3780,4000, main_jeb1))),
	  make_pair(make_pair("main_map",new door(1560,9960,1735,480,1800,650, common_main)), make_pair("commons", new door(1680,520,1480,10033,1750,10200, common_main))),
	  make_pair(make_pair("jeb1",new door(340,2000,280,2160,320,2200, jeb1_jeb2_bottom)), make_pair("jeb2", new door(360,2040,360,2070,400,2160, jeb1_jeb2_bottom))),//jeb1 bottom to jeb2 bottom
	  make_pair(make_pair("jeb1",new door(200,600,54,480,127,560, jeb1_jeb2_left)), make_pair("jeb2", new door(200, 560,50,640,140,720, jeb1_jeb2_left))), //jeb1 left to jeb2 left
	  make_pair(make_pair("jeb2",new door(360,2040,280,2000,320,2150, jeb2_jeb3_bottom)), make_pair("jeb3", new door(340,2000,360,2160,400,2200, jeb2_jeb3_bottom))),//jeb2 bottom to jeb3 bottom
	  make_pair(make_pair("jeb2",new door(200,560,80,480,140,555, jeb2_jeb3_left)), make_pair("jeb3", new door(200,600,54,625,127,700, jeb2_jeb3_left)))//jeb2 left to jeb3 left
	  */
  };

  pMapGraph = new MapGraph(allmaps, doorpairs);
  
  string shoes[8] = { "shoe1", "shoe2", "shoe3", "shoe4", "shoe5", "shoe6", "shoe7", "shoe8" };

  Player* engineer;
  Player* guardsman;
  Player* chemist;
  Player* philosopher;
  engineer=new Player(this,"Engineer",pMapGraph->getMap("main_map"),player_female_moving, player_female_stopping,1, 4380,180, 2, true, 0.1 / 3.0f, "shoe1");//initialize to main_map (4438,165) absolute pixel coordinates in main_map.bmp
  sprites.push_back(engineer);
  guardsman = new Player(this, "Guardsman", pMapGraph->getMap("main_map"), player_male_moving, player_male_stopping, 1, 4350, 240, 2, true, 0.1 / 3.0f, "shoe2");
  chemist=new Player(this,"Chemist",pMapGraph->getMap("main_map"),player_female_moving, player_female_stopping,1, 4440,160, 2, true, 0.1 / 3.0f, "shoe3");
  philosopher=new Player(this,"Philosopher",pMapGraph->getMap("main_map"),player_male_moving, player_male_stopping,1, 4440,280, 2, true, 0.1 / 3.0f, "shoe4");


  NPC* tourguide=new NPC(this,namepool[rand()%30],"TourGuide",false,0, pMapGraph->getMap("main_map"), 4127, 264,3, 1, goggles_moving, goggles_stopping,true, goggles_stopping[3][0], 0.1 / 3.0f,true, "shoe5");
  NPC* jint=new NPC(this,"Jint Clevery","Boss",false,0, pMapGraph->getMap("jeb2"), 186, 1650, 2, 10, jint_moving, jint_stopping,false, jint_stopping[2][0], 0.1 / 2.0f,true, "shoe6");
  NPC* police=new NPC(this,namepool[rand()%30], "Police", false,0,pMapGraph->getMap("main_map"),1481, 9857,1, 1, police_moving, police_stopping,false, police_stopping[1][0], 0.1 / 2.0f,true, "shoe7");

  NPC* mob1=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),33, 16, rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob2=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),26, 727,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob3=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),600, 241,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob4=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),430, 3360,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob5=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),36, 4790,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob6=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),607, 5801,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob7=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),4125, 9606,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob8=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),2467, 12241,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);
  NPC* mob9=new NPC(this,namepool[rand()%30],"Mob",true,1,pMapGraph->getMap("main_map"),3306, 12273,rand() % 4, 1, mob_moving,mob_stopping,true,nullptr, 0.1 / 2.0f,true, shoes[rand() % 8]);


  sprites.push_back(guardsman);
  sprites.push_back(chemist);
  sprites.push_back(philosopher);
  sprites.push_back(tourguide);
  sprites.push_back(jint);
  sprites.push_back(police);
  sprites.push_back(mob1);
  sprites.push_back(mob2);
  sprites.push_back(mob3);
  sprites.push_back(mob4);
  sprites.push_back(mob5);
  sprites.push_back(mob6);
  sprites.push_back(mob7);
  sprites.push_back(mob8);
  sprites.push_back(mob9);

  pIntro = new Sound(pSoundDevice, ".//Sound//BGM//RoadtripWithNo.wav", 0);
  pNextPage = new Sound(pSoundDevice, ".//Sound//Effects//nextpage.wav", 0);
  pButtonClose = new Sound(pSoundDevice, ".//Sound//Effects//buttonclose.wav", 0);
  pButtonOpen = new Sound(pSoundDevice, ".//Sound//Effects//scrollpaper.wav", 0);
  pExplore = static_cast<Player*>(sprites[0])->getWalkEngine();
  targetX=103;
  targetY=49;



  pIntro->Play();
}

 Core::~Core(){
	 delete pBattle;
	 //delete pExplore;-->character destructor will delete every walkengine including 4 player's explore class
	 delete pGraphic; 
	 delete pMapGraph;
	 delete pButtonOpen;
	 delete pButtonClose;
	 delete pNextPage;
	 delete noticequeue;
	 delete teampop;
	 delete mouseripple;
	 delete pSoundDevice;

	 for (auto sprite : sprites) {
		 delete sprite;
		 sprite = nullptr;
	 }

 }