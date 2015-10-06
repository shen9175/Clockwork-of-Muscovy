#ifndef CORE_H
#define CORE_H

using namespace std;
//#define DEBUG
//#define DEBUGMAPCHANGE
class Core {
	friend class Explore;
	friend class Battle;
	friend class Battlehelper;
	friend class Graphic;
	friend class Character;
	friend class Player;//friend of base class does not affect it's derive class
	friend class Inventory;
	friend class Major;
	friend class WalkEngine;

public:
	Core(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) : hInstance(hInstance), hPrevInstance(hPrevInstance), szCmdLine(szCmdLine), iCmdShow(iCmdShow) { singlestate = 2; }
	~Core();
	int WINAPI run();
private:
	Battle* pBattle;
	Explore* pExplore;
	Graphic* pGraphic;
	MapGraph* pMapGraph;
	IXAudio2* pXAudio2;
	IXAudio2MasteringVoice* pMasterVoice;
	IXAudio2SubmixVoice* pSubmixVoice;
	Sound* pButtonOpen;
	Sound* pButtonClose;
	Sound* pDoor;
	Sound* pIntro;
	Sound* pNextPage;
	Sound* pNotice;
	Animation* mouseripple;
	std::string dialogue[100];
	std::string current_dialogue; 
	HBITMAP journal[7];
	int dindex;
	NoticeQueue* noticequeue;//shared information by Battle, Graphic, Explore, Character classes
	TeamPop *teampop;//definition in Graphic class .cpp/.h
	int currentTaskID;
	int currentMonitoredNPC;

	//=============shared global variables start===============//
	std::vector<Character*> sprites;
	HBITMAP hbitmap[346];

	HBITMAP picture;
	HBITMAP current_journal;
	HBITMAP player_female_stopping[4][1];
	HBITMAP player_female_moving[4][3];
	HBITMAP player_female_attacking[4][2];
	HBITMAP player_female_deading[4][2];
	HBITMAP player_female_dead[4][1];
	HBITMAP player_female_magic[4][1];
	HBITMAP player_female_pistol[4][3];
	HBITMAP player_female_hit[4][2];
	HBITMAP player_female_active[4][1];
	HBITMAP player_female_grenade[4][2];


	HBITMAP player_male_stopping[4][1];
	HBITMAP player_male_moving[4][3];
	HBITMAP player_male_attacking[4][2];
	HBITMAP player_male_deading[4][2];
	HBITMAP player_male_dead[4][1];
	HBITMAP player_male_magic[4][1];
	HBITMAP player_male_pistol[4][3];
	HBITMAP player_male_hit[4][2];
	HBITMAP player_male_active[4][1];
	HBITMAP player_male_grenade[4][2];


	HBITMAP mob_stopping[4][1];
	HBITMAP mob_moving[4][3];
	HBITMAP mob_attacking[4][2];
	HBITMAP mob_deading[4][2];
	HBITMAP mob_dead[4][1];
	HBITMAP mob_hit[4][2];

	HBITMAP goggles_stopping[4][1];
	HBITMAP goggles_moving[4][3];
	HBITMAP police_stopping[4][1];
	HBITMAP police_moving[4][3];
	HBITMAP jint_stopping[4][1];
	HBITMAP jint_moving[4][3];

	HBITMAP mouse_ripple[1][3];
	
	XAUDIO2FX_REVERB_I3DL2_PARAMETERS I3DL2_Reverb[30] =
	{
		XAUDIO2FX_I3DL2_PRESET_DEFAULT,
		XAUDIO2FX_I3DL2_PRESET_GENERIC,
		XAUDIO2FX_I3DL2_PRESET_FOREST,
		XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
		XAUDIO2FX_I3DL2_PRESET_ROOM,
		XAUDIO2FX_I3DL2_PRESET_BATHROOM,
		XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
		XAUDIO2FX_I3DL2_PRESET_STONEROOM,
		XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
		XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
		XAUDIO2FX_I3DL2_PRESET_CAVE,
		XAUDIO2FX_I3DL2_PRESET_ARENA,
		XAUDIO2FX_I3DL2_PRESET_HANGAR,
		XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
		XAUDIO2FX_I3DL2_PRESET_HALLWAY,
		XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
		XAUDIO2FX_I3DL2_PRESET_ALLEY,
		XAUDIO2FX_I3DL2_PRESET_CITY,
		XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
		XAUDIO2FX_I3DL2_PRESET_QUARRY,
		XAUDIO2FX_I3DL2_PRESET_PLAIN,
		XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
		XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
		XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
		XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
		XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
		XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
		XAUDIO2FX_I3DL2_PRESET_PLATE,
	};

	const char* I3DL2_Name[ 30 ] =
	{
		"Default",
		"Generic",
		"Forest",
		"Padded cell",
		"Room",
		"Bathroom",
		"Living room",
		"Stone room",
		"Auditorium",
		"Concert hall",
		"Cave",
		"Arena",
		"Hangar",
		"Carpeted hallway",
		"Hallway",
		"Stone Corridor",
		"Alley",
		"City",
		"Mountains",
		"Quarry",
		"Plain",
		"Parking lot",
		"Sewer pipe",
		"Underwater",
		"Small room",
		"Medium room",
		"Large room",
		"Medium hall",
		"Large hall",
		"Plate",
	};

	//========main.cpp shared global variables definitions start==============//
	HWND  hwnd; // main window handle
	HWND  hChild;//game screen window handl
	HWND  hwndAttckBT;     // Handle to ATTACK button
	HWND  hwndMoveBT;		// Handle to Move button
	HWND  hwndUseItemBT;	// Handle to use item button
	HWND  hwndUseSkillBT;	// Handle to use skill button
	HWND  hwndItemListBox;  // Handle to Item list box
	HWND  hwndJornalBT;		// Handle to Jornal button
	HWND  hwndSkillDropBox; // Handle to Skill drop box
	HWND  hwndMapBT;		// Handle to Map button
	HWND  hwndStatusBT;	// Handle to Status Button

	
	//========main.cpp shared global variables definitions end==============//



	/**************main.cpp dedicated global variables definitions**************/
	int mode;
	bool mouseclick;
	int targetX,targetY;//small map quest target icon position
	int currentX,currentY;//mini map player current icon position
	std::string whichbutton;
	int userchoice;//battle mode button choice
	int ycounter = 0;
	bool jornalclicked;
	bool mapclicked;
	bool statusclicked;
	bool f1pressed;
	bool f2pressed;
	bool f3pressed;
	bool f4pressed;
	bool f5pressed;
	bool f6pressed;
	bool f7pressed;
	float globalBGMVolume;
	int singlestate; // 1. spreading, 2. spread, 3. gathering, 4. gathered
	int I3DL2 = 0;
	Timer gameTimer;
	Timer BGMVolDispTimer;
	bool BGMVolDispSwitch;
	pair<bool, int> antiAround;
	





	HINSTANCE    hInstance;
	HINSTANCE hPrevInstance;
	PSTR szCmdLine;
	int iCmdShow;
	HRESULT hr;

	//========battle.cpp shared global variables definitions start==============//
	int itemindex;
	int skillindex;
	int currentPlayer;
	std::vector<int> mobs;
	int n_mobs;
	//========battle.cpp shared global variables definitions end==============//


	void dialogueupdate(HWND);
	void dialogue_initial();
	void initialGame();
	void GameControl();
	void initialWindow();

	static LRESULT CALLBACK RetriveWndProcPointer(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK RetriveChildWndProcPointer(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
	LRESULT CALLBACK ChildWndProc (HWND, UINT, WPARAM, LPARAM);
	LRESULT OnMainCreate(HWND Wnd);
	LRESULT OnMainActivate(WPARAM wParam);
	LRESULT OnMainPaint(HWND hwnd);
	LRESULT OnMainLBUTTONDOWN(HWND hwnd);
	LRESULT OnMainCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnChildMouseMove(HWND hwnd, LPARAM lParam);
	LRESULT OnChildLBUTTONDOWN(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnChildPaint(HWND hwnd, HDC hdc);
	LRESULT OnKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseWheel(WPARAM wParam, LPARAM lParam);
};

#endif


