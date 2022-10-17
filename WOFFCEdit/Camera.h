#pragma once
#include "pch.h"
#include "InputCommands.h"
#include "SimpleMath.h"

class Camera
{
public:

	Camera();
	~Camera();

	DirectX::SimpleMath::Vector3 getPosition();
	DirectX::SimpleMath::Vector3 getOrientation();
	DirectX::SimpleMath::Vector3 getLookAt();
	DirectX::SimpleMath::Vector3 getLookDirection();
	DirectX::SimpleMath::Vector3 getRight();
	float getRotRate();
	float getMoveSpeed();
	
	void setPosition(DirectX::SimpleMath::Vector3 newPosition);
	void setOrientation(DirectX::SimpleMath::Vector3 newOrientation);
	void setLookAt(DirectX::SimpleMath::Vector3 newLookAt);
	void setLookDirection(DirectX::SimpleMath::Vector3 newLookDirection);
	void SetRotRate(float newRate);
	void SetMoveSpeed(float newSpeed);

	void Update(InputCommands Input);


private:

	DirectX::SimpleMath::Vector3		position;
	DirectX::SimpleMath::Vector3		orientation;
	DirectX::SimpleMath::Vector3		lookAt;
	DirectX::SimpleMath::Vector3		lookDirection;
	DirectX::SimpleMath::Vector3		rightVector;
	float rotRate;
	float moveSpeed;

};