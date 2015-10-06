#ifndef TIMER_H
#define TIMER_H

class Timer {
	public:
		Timer() { mDeltaTime = -1.0; mBaseTime = 0; mPausedTime = 0; mPrevTime = 0; mCurrTime = 0; mStopTime = 0; mStopped = false; mTicks = 0; mTotalTicks = 0; }
		~Timer() {}
		double ElapsedTime() const; //return total effective time of gaming--> current time called this function substract(-) the most recent time called Reset() then substract(-) the pause time during this time interval.
		double DeltaTime() const;//return total time (time interval) from last time called Tick() function and this time called DeltaTime() function
		void Reset();//restart a total new time record. The time called Reset() will be the baseTime(the most first/earliest time in this Timer usage round)
		void Resume();//resume recording time from Pause status. If the game is in the pause status(Stop() called before), then this function will resume the pause and continue to record the time. if it is not in pause status, nothing happens.
		void Pause();//Pause the time recording.
		void Tick();//every time called Tick(), it will record the new current time and update the previous time with old current time. And calculate the updated delta time. Frames increment.
		unsigned long long TotalTicks() const{ return mTotalTicks; } //return total ticks start from last Reset() called. usually used for frame count. each frame tick one time.
	private:
		double mDeltaTime; //time interval: total time (time interval) from last time called Tick() function and this time called DeltaTime() function
		clock_t mBaseTime;//time point: the most first/earliest time in this Timer usage round
		clock_t mPausedTime;//time interval: total time between all corresponding Pause()~Resume() interval (accumalted time intervals)
		clock_t mStopTime; //time point: the most recent time point when the functio Stop() called;
		clock_t mPrevTime; //last time point when Tick() function called
		clock_t mCurrTime; //the current time point when Tick() function called
		bool mStopped; // flag indicat if the timer is in pause status
		unsigned long long mTicks;//increment by 1 when Tick() called
		unsigned long long mTotalTicks;// total Tick() calls between two Reset() called. The second Reset() will record total ticks starting from last Reset() and keep it unchanged until next Reset(). It usually used for calculating average FPS.
};
#endif

//About timing system using in this game:

//There are 4 timing systems (4 timers) in this game:
//1.gameTimer-->record the real playing the game, pause time in seconds.
//2.Message notice display time control timer-->control the message display time on screen. It is a member in the noticequeue class.
//3.AnitmationTimer-->control the animation frame speed: how much time of each animation frame.
//4.fps timer-->calculating the game render speed: how many total frames rendered per second.

//The animation frame speed is different from render frame speed-->FPS.
//FPS is the total frames rendered per second. Usually there is no waiting time between each frame. It can be a large number(100~200+) due to the machine speed.
//Animation frame speed is pre-determined by game/animation designer. If a set of animation (for example: moving) has 3 frames. If we want the moving animation using 0.3s finish playing this set
//Then each animation frame cost 0.3s/3 = 0.1s to played. To be exactly speaking, the time interval starts from the first frame drawing to the time just before the second frame drawing is 0.1s
//If the drawing/rendering a frame costs very short time, then we can assume there is 0s for drawing one frame.
//Then animation speed 0.1s per frame can be considered that there is 0.1s time waiting between two frame drawing/rendering-->in this game, we assume there is 0s for rendering a frame
//So animation timer is control the waiting time between two animation frames. We set this time according to the origianl animation speed designed.

//If the game FPS is unlocked, FPS and animation uses two different timers. There is no waiting between two frames rendering while there usually be some waiting time between two animation frames
//So FPS could be much more than 60+, but animation will not be that fast even FPS is very high
//If them game FPS is locked for example 30fps/60fps. Then FPS and animation could use the same timers, if animation designed in 30fps/60fps.
//If animation designed not in 30fps/60fps then we still have to use 2 timers.
//In locked FPS mode, we intended set some waiting time between two rendering frames, if two frames rendered less than (1/30s or 1/60s) to fullfill the locked FPS
//Usually rendering and internal data/control updating are in the same game loop, if we intended set some waiting time between two rendering frames, then the control responsitivity become worse.


//About showing FPS using this Timer Class note:
//the instant FPS can be calculated by (1.0 / time per frame in second) and second per frame can get from the DeltaTime between two Tick() called. --> Tick() call stays in the game Render loop
//So there is one Render frame between two Tick() calls. Then the deltaTime is the time(sec) per frame. And it's reciprocal (1.0/deltaTime) is instant FPS
//But showing this instant FPS every frame in the screen, it become too flickery since it changes every frame (usually in millisecond magtitude), people will get uncomfortable due to flickery.
//So we came to anther approach, calculating the average frames per second. Count and record the total ticks(frames) in 1 second and let the screen keep showing last second total frames
//Or we can count the total frames in last n seconds and let screen keep show last n second's (total frames / n seconds) every n seconds---> real average

/*
first fps.Reset() called in the beginning of the game. or we do not call it which the first second total frames (first FPS) will be 0 due to the constructor set mTotalTicks = 0;
=================================
if (fpsTimer.ElapsedTime() >= 1.0f) {
fpsTimer.Reset();
}
fpsTimer.Tick();

RECT textRect;
char buffer[256];
SetBkMode(hdcMem, TRANSPARENT);
SetRect(&textRect, 0, 0, 300, 20);
sprintf_s(buffer, "%lld %s", fpsTimer.TotalTicks(), "Frame Per Second");
DrawText(hdcMem, buffer, -1, &textRect, DT_LEFT | DT_VCENTER);
*/
