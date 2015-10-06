#ifndef ANIMATION_H
#define ANIMATION_H

class Animation {
public:
	Animation(HBITMAP sprites[][3], int directions, int frames, float timeTofinish);
	~Animation();
	void Update();
	HBITMAP getCurrentSprite() { return animation[orientation][currentFrame]; }
	void setDirection(int dir) { currentDirection = dir; }
	void oneTimeDraw(HDC hdcMem);
	void oneTimeDrawStart(int x, int y, int w, int h, Camera* cam);
private:
	Timer animationTimer;
	int orientation;
	int totalframes;
	float time;
	HBITMAP (*animation)[3];
	int counter;
	int currentFrame;
	int currentDirection;
	bool oneTimeDrawLatch;
	int oneTimeDraw_x;
	int oneTimeDraw_y;
	int oneTimeDraw_w;
	int oneTimeDraw_h;
	Camera* camera;
};

#endif
