#pragma once

#include "glm/glm.hpp"

class Camera {
public:
	Camera(glm::vec3 pPos, glm::vec3 pFront, glm::vec3 pUp, float pSensitivity, float pFov, float pSpeed);
	~Camera();

	void update(float deltaTime);

	void setPosition(glm::vec3 pPos);
	void setFront(glm::vec3 pFront);
	void setUp(glm::vec3 pUp);

	void setYaw(float pYaw);
	void setPitch(float pPitch);

	glm::vec3 getPosition();
	glm::vec3 getLockedPosition();
	glm::vec3 getFront();
	glm::vec3 getUp();
	glm::vec3 getRelativeUp();
	glm::vec3 getRight();

	glm::mat4 getViewMatrix();
	float getFov();

	void setLocked(bool pLocked);
	bool getLocked();

	void setSpeed(float pSpeed);
	void translate(glm::vec3 pDirection);
	
private:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;

	glm::vec3 lockedPosition;
	glm::vec3 lockedFront;
	glm::vec3 lockedUp;
	bool locked;

	float sensitivity;
	float fov;
	float speed;

	float yaw;
	float pitch;
};