#pragma once

struct InputCommands
{
	//For camera controls
	//Camera movement
	bool forward;
	bool back;
	bool right;
	bool left;
	//Camera aiming
	bool rotRight;
	bool rotLeft;
	bool rotUp;
	bool rotDown;
	bool cameraDrag;
	
	//For left mouse button
	bool mouseLDown;
	bool mouseLClick;

	//For multiple selections
	bool multiPick;

	//For copying
	bool copySelection;
	
	//For object rotation
	bool rotDragX;
	bool rotDragY;
	bool rotDragZ;

	//For object scaling
	bool scaleDragX;
	bool scaleDragY;
	bool scaleDragZ;

	//For mouse controls
	//Mouse current position
	float mousePosX;
	float mousePosY;
	//Mouse previous position
	float mousePosPrevX;
	float mousePosPrevY;
	//Amount mouse has moved
	float mouseSpeedX;
	float mouseSpeedY;

	//For object rotation
	bool rotObjXPos;
	bool rotObjXNeg;
	bool rotObjYPos;
	bool rotObjYNeg;
	bool rotObjZPos;
	bool rotObjZNeg;

	//For object scaling
	bool scaleObjXUp;
	bool scaleObjXDown;
	bool scaleObjYUp;
	bool scaleObjYDown;
	bool scaleObjZUp;
	bool scaleObjZDown;

	//For object deletion
	bool deleteButton;
	bool deleteButtonPressed;

	//For saving the scene
	bool saveButton;
	bool saveButtonPressed;
};
