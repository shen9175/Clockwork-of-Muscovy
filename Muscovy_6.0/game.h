#ifndef GAME_H
#define GAME_H

#define ResX 1024 //game screen width is 1024 pixel
#define ResY 600 //game screen height is 600 pixel

#define ATTACKBUTTON 1001
#define ITEMLISTBOX 1002
#define MOVEBUTTON  1003
#define USEITEMBUTTON 1004
#define USESKILLBUTTON 1005
#define SKILLDROPBOX 1006
#define JOURNALBUTTON 1007
#define MAPBUTTON 1008
#define STATUSBUTTON 1009

class NoticeQueue {
public:
	NoticeQueue(IXAudio2* paudio) { p = new Sound(paudio, ".//Sound//Effects//notice.wav", 0); noticeTimer.Reset(); bShowing = false; }
	~NoticeQueue() { delete p; }
	double getElapsedTime() {return noticeTimer.ElapsedTime();}
	void resetTimer() { noticeTimer.Reset(); }
	void push(std::string notice) {q.push(notice);}
	std::string front() { return q.front(); }
	void playSound() {p->Play();}
	void pop() {q.pop();}
	bool empty() {return q.empty();}
	bool showing() { return bShowing; }
	void setShowing(bool s) { bShowing = s; }
private:
	Sound* p;
	std::queue<std::string> q;
	Timer noticeTimer;
	bool bShowing;
};

#endif