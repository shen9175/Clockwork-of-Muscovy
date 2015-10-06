#ifndef MOVE_H
#define MOVE_H
class Character;
class Move {
public:
	Move(Character* sprite, Map*&inmap, int x, int y);
	~Move();
	void MoveControl();
	bool whichmoving(int X, int Y, int deltaRow, int deltaCol);
	void NormalMoving();
	void TwoPartsMoving();
	void FinalPixelDelivery();
	void FindPath();
	void incrementFrame() { ++FrameCounter; }
	void setFrameCounter(int inner_c) { FrameCounter = inner_c; }
	int getHorizontalMovementPiece() const { return ColMovePiece; }
	int getVerticalMovementPiece() const { return RowMovePiece; }
	void setDestination(int x, int y) { MouseX = x; MouseY = y; }
	bool finish() { return finishMoving; }

private:
	
	int FrameCounter;
	int movingStatus; //1-normal moving 2-two parts moving 3-FinalPixelDelivery
	int currRow;
	int currCol;
	int nextRow;
	int nextCol;
	int RowMovePiece, ColMovePiece;////each moving step(frame) moves in the map (RowMovePiece, ColMovePiece) pixel distance
	int twoPartsMovingTargetX, twoPartsMovingTargetY;
	bool TwoPartsMovingfirstHalf;
	int twoPartsMoving1stVertexX, twoPartsMoving1stVertexY;
	int twoPartsMoving2ndVertexX, twoPartsMoving2ndVertexY;
	nodeptr path;
	Character* player;
	Map* &map;
	int MouseX, MouseY;
	bool finishMoving;
};
#endif