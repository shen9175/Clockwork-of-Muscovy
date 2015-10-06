#ifndef GRAPHICS_H
#define GRAPHICS_H
class Core;
class ScreenPop {
public:
	ScreenPop();
	ScreenPop(int l, int t, int w, int h, float speed);
	//finite state machine for screen rolling out and in animation by F5 key toggling ON/OFF
	void MapPoping(int& l, int&  t, int& w, int& h, bool toggle, Map* map);
	void Poping(int& l, int&  t, int& w, int& h, bool toggle);
private:
	int state;// 1. opening, 2. open, 3. closing, 4. close
	int counter;
	int left;
	int top;
	int width;
	int height;
	float rollingspeed;//how long the screen finishing rolling out: unit(second)
	Timer AniTimer;
};
class TeamPop {
	public:
		TeamPop(IXAudio2* paudio, int &singlestate) : state(singlestate) { AniTimer.Reset(); firstime = true; counter = 10; soundfx = new Sound(paudio, ".//Sound//Effects//spreading.wav", 0); }//initial is spread mode
		void TeamPoping(vector<Character*> sprites, bool toggle);
	private:
		int& state;
		Timer AniTimer;
		int counter;
		bool firstime;
		Sound* soundfx;
		int GuardsmanDeltaX;
		int GuardsmanDeltaY;
		int ChemistDeltaX;
		int ChemistDeltaY;
		int PhilosopherDeltaX;
		int PhilosopherDeltaY;
};
class Graphic {
	public:
		Graphic(Core* p) : pCore(p) { fpsTimer.Reset(); }
		IStream * CreateStreamOnResource(int resourceID, LPCSTR lpType);
		IWICBitmapSource * LoadBitmapFromStream(IStream* ipImageStream);
		HBITMAP CreateHBITMAP(IWICBitmapSource* ipBitmap);
		HBITMAP LoadSplashImage(int resourceID);
		void DrawScreen(HWND hWnd,  HBITMAP pic,int screenX, int screenY, int screenW, int screenH,int picX, int picY,int picW, int picH);
		void DrawScreen(HWND hWnd, Character* p);//the main Camera focus on the main character projects on the main screen
		void DrawScreen(HDC hdcMem,Character* p, int left, int top, int width, int height);//small Camera focus selected character projects to specific place on the whole screen
		void DrawScreen(HWND hWnd,  HBITMAP sprites, HBITMAP background,int spritesX, int spritesY,int backGroundX, int backGroundY);
		void DrawMapMode(HWND hWnd, Character* p, HBITMAP target, HBITMAP player,HBITMAP smallmap,int mapX, int mapY,  int targetX, int targetY,int playerX, int playerY);
		void DrawRealTimeMap(HDC hdcfront, Character* p, int left, int top, int width, int height);
		void ShowStat(HWND hWnd);
	//void shadeAvailable(vector<Position>);
private:
	Timer fpsTimer;
	ScreenPop secondScreen;
	ScreenPop smallMap { ScreenPop(824, 45, 200, 555, 1.0f) };//ScreenPop(916, 300, 108, 300, 1.0f)
	Core* pCore;
	
};
//HBITMAP hBitmap;



#endif
