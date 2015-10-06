#include <ctime>
#include "timer.h"
using namespace std;


void Timer::Tick() {
	if (mStopped) {
		mDeltaTime = 0.0f; // if the timer is in stop status, then there is no delta time
		return;
	}
	mCurrTime = clock();
	mDeltaTime = (mCurrTime - mPrevTime) / static_cast<double>(CLOCKS_PER_SEC);
	mPrevTime = mCurrTime; // update the previous time point
	//if the processor goes into a power save mode or we get shuffled to another processor, then mDeltaTime can be negative
	if (mDeltaTime < 0.0f) {
		mDeltaTime = 0.0f;
	}
	++mTicks;
}
double Timer::DeltaTime() const {
	return mDeltaTime;
}

void Timer::Reset() {
	mCurrTime = clock();
	mBaseTime = mCurrTime;
	mPrevTime = mCurrTime;
	mStopped = false;
	mStopTime = 0;
	mTotalTicks = mTicks;//record total ticks called from last reset
	mTicks = 0;
}

void Timer::Pause() {
	if (!mStopped) {
		mCurrTime = clock();
		mStopTime = mCurrTime;
		mStopped = true;
	}
}

void Timer::Resume() {
	if (mStopped) {
		mCurrTime = clock();
		mPausedTime += mCurrTime - mStopTime; //only record how many CPU ticks not the real time
		mStopTime = 0;
		mStopped = false;
		mPrevTime = mCurrTime;
	}
}
double Timer::ElapsedTime() const{
	if (mStopped) {
		return static_cast<double>((mStopTime - mBaseTime - mPausedTime) / static_cast<double>(CLOCKS_PER_SEC));
	} else {
		return static_cast<double>((clock() - mBaseTime - mPausedTime) / static_cast<double>(CLOCKS_PER_SEC));
	}
}
