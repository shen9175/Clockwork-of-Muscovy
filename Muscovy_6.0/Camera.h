#ifndef CAMERA_H
#define CAMERA_H
class Camera {		//2D scrolling screen camera
public:
	Camera(int MapWidth, int MapHeight, int playerX, int playerY);
	Camera(Map* map, int playerX, int playerY);
	void setCamera(int MapWidth, int MapHeight, int playerX, int playerY);
	void setCamera(Map* map, int playerX, int playerY);
	void CameraUpdate(int MapWidth, int MapHeight, int playerX, int playerY, int deltaX, int deltaY);
	int getLeft() const { return left; }
	int getTop() const { return top; }
	int getRight() const { return right; }
	int getBottom() const { return bottom; }

private:
	int left; //The x-coordinate of the upper-left corner of the camera rectangle.
	int	top; //The y-coordinate of the upper-left corner of the rectangle.
	int right; //The x-coordinate of the lower-right corner of the rectangle.
	int bottom; //The y-coordinate of the lower-right corner of the rectangle.
};
#endif
