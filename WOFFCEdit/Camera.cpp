#include "Camera.h"

using namespace DirectX::SimpleMath;

Camera::Camera()
{
	moveSpeed = 0.30;
	rotRate = 3.0;

	position.x = 0.0f;
	position.y = 3.7f;
	position.z = -3.5f;

	lookAt.x = 0.0f;
	lookAt.y = 0.0f;
	lookAt.z = 0.0f;

	lookDirection.x = 0.0f;
	lookDirection.y = 0.0f;
	lookDirection.z = 0.0f;

	rightVector.x = 0.0f;
	rightVector.y = 0.0f;
	rightVector.z = 0.0f;

	orientation.x = 0.0f;
	orientation.y = 0.0f;
	orientation.z = 0.0f;
}

Camera::~Camera()
{

}

DirectX::SimpleMath::Vector3 Camera::getPosition()
{
	return position;
}

DirectX::SimpleMath::Vector3 Camera::getOrientation()
{
	return orientation;
}

DirectX::SimpleMath::Vector3 Camera::getLookAt()
{
	return lookAt;
}

DirectX::SimpleMath::Vector3 Camera::getLookDirection()
{
	return lookDirection;
}

DirectX::SimpleMath::Vector3 Camera::getRight()
{
	return rightVector;
}

float Camera::getRotRate()
{
	return rotRate;
}

float Camera::getMoveSpeed()
{
	return moveSpeed;
}

void Camera::setPosition(DirectX::SimpleMath::Vector3 newPosition)
{
	position = newPosition;
}

void Camera::setOrientation(DirectX::SimpleMath::Vector3 newOrientation)
{
	orientation = newOrientation;
}

void Camera::setLookAt(DirectX::SimpleMath::Vector3 newLookAt)
{
	lookAt = newLookAt;
}

void Camera::setLookDirection(DirectX::SimpleMath::Vector3 newLookDirection)
{
	lookDirection = newLookDirection;
}

void Camera::SetRotRate(float newRate)
{
	rotRate = newRate;
}

void Camera::SetMoveSpeed(float newSpeed)
{
	moveSpeed = newSpeed;
}

void Camera::Update(InputCommands Input)
{
	//Camera rotation
	if (Input.rotRight)
	{
		orientation.y -= rotRate * Input.mouseSpeedX;
	}
	if (Input.rotLeft)
	{
		orientation.y += rotRate * Input.mouseSpeedX;
	}
	if (Input.rotDown)
	{
		if (orientation.x > -85)
		{
			orientation.x -= rotRate * Input.mouseSpeedY;
		}
	}
	if (Input.rotUp)
	{
		if (orientation.x < 85)
		{
			orientation.x += rotRate * Input.mouseSpeedY;
		}
	}

	//create look direction from Euler angles
	lookDirection.x = sin((orientation.y) * 3.1415 / 180) * cos((orientation.x) * 3.1415 / 180);
	lookDirection.z = cos((orientation.y) * 3.1415 / 180) * cos((orientation.x) * 3.1415 / 180);
	lookDirection.y = sin((orientation.x) * 3.1415 / 180);
	lookDirection.Normalize();

	//create right vector from look Direction
	lookDirection.Cross(Vector3::UnitY, rightVector);
	rightVector.Normalize();

	//Camera movement
	if (Input.forward)
	{
		position += lookDirection * moveSpeed;
	}
	if (Input.back)
	{
		position -= lookDirection * moveSpeed;
	}
	if (Input.right)
	{
		position += rightVector * moveSpeed;
	}
	if (Input.left)
	{
		position -= rightVector * moveSpeed;
	}

	//update lookat point
	lookAt = position + lookDirection;
}