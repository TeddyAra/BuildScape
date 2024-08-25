#include "Camera.h"

#include "Input.h"
#include "glm/gtc/matrix_transform.hpp"

Camera::Camera(glm::vec3 pPos, glm::vec3 pFront, glm::vec3 pUp, float pSensitivity, float pFov, float pSpeed, float pRange)
	: position(pPos), front(pFront), up(pUp), sensitivity(pSensitivity), fov(pFov), speed(pSpeed), range(pRange), locked(false)
{

}

Camera::~Camera() {

}

void Camera::update(float deltaTime) {
	// Get difference in mouse position and calculate yaw and pitch
	glm::vec2 delta = Input::getDeltaMousePosition();
	delta *= sensitivity * deltaTime;
	yaw += delta.x;
	pitch -= delta.y;

	// Clamp pitch
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	// Update front
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);
}

void Camera::setPosition(glm::vec3 pPos) {
	position = pPos;
}

void Camera::setFront(glm::vec3 pFront) {
	front = pFront;
}

void Camera::setUp(glm::vec3 pUp) {
	up = pUp;
}

void Camera::setYaw(float pYaw) {
	yaw = pYaw;
}

void Camera::setPitch(float pPitch) {
	pitch = pPitch;
}

glm::vec3 Camera::getPosition() {
	return position;
}

glm::vec3 Camera::getLockedPosition() {
	return lockedPosition;
}

glm::vec3 Camera::getFront() {
	return front;
}

glm::vec3 Camera::getUp() {
	return up;
}

glm::vec3 Camera::getRelativeUp() {
	glm::vec3 relativeUp = glm::normalize(glm::cross(getRight(), front));
	return relativeUp;
}

glm::vec3 Camera::getRight() {
	glm::vec3 right = glm::normalize(glm::cross(front, up));
	return right;
}

glm::mat4 Camera::getViewMatrix() {
	glm::mat4 viewMatrix = glm::lookAt(position, position + front, up);
	return viewMatrix;
}

float Camera::getFov() {
	return fov;
}

void Camera::setLocked(bool pLocked) {
	locked = pLocked;

	if (locked) {
		lockedPosition = position;
		lockedFront = front;
		lockedUp = up;
	}
}

bool Camera::getLocked() {
	return locked;
}

void Camera::setSpeed(float pSpeed) {
	speed = pSpeed;
}

void Camera::translate(glm::vec3 pDirection) {
	position += pDirection * speed;
}

Camera::IntersectionInfo Camera::checkIntersection(float pVoxelSize, glm::vec3 pPosition, std::uint32_t pBlock) {
	IntersectionInfo info;

	float voxel = pVoxelSize / 2.0f;
	constexpr float epsilon = std::numeric_limits<float>::epsilon();
	glm::vec3 camPos = position * (1.0f / pVoxelSize);

	bool withinX = (camPos.x >= pPosition.x - voxel - epsilon) && (camPos.x <= pPosition.x + voxel + epsilon);
	bool withinY = (camPos.y >= pPosition.y - voxel - epsilon) && (camPos.y <= pPosition.y + voxel + epsilon);
	bool withinZ = (camPos.z >= pPosition.z - voxel - epsilon) && (camPos.z <= pPosition.z + voxel + epsilon);

	if (withinX && withinY && withinZ) {
		info.intersected = true;
		info.inside = true;
		return info;
	}
		
	glm::vec3 tMin;
	glm::vec3 tMax;
	glm::vec3 aabbMin = pPosition - glm::vec3(voxel, voxel, voxel);
	glm::vec3 aabbMax = pPosition + glm::vec3(voxel, voxel, voxel);

	for (int i = 0; i < 3; ++i) {
		if (front[i] != 0.0f) {
			tMin[i] = (aabbMin[i] - camPos[i]) / front[i];
			tMax[i] = (aabbMax[i] - camPos[i]) / front[i];

			if (tMin[i] > tMax[i]) std::swap(tMin[i], tMax[i]);
		} else {
			tMin[i] = -std::numeric_limits<float>::infinity();
			tMax[i] = std::numeric_limits<float>::infinity();
		}
	}

	float tEntry = std::max(std::max(tMin[0], tMin[1]), tMin[2]);
	float tExit = std::min(std::min(tMax[0], tMax[1]), tMax[2]);

	if (tEntry > tExit || tExit < 0) {
		return info;
	}

	int hitAxis = -1;
	if (tEntry == tMin[0]) hitAxis = 0;
	else if (tEntry == tMin[1]) hitAxis = 1;
	else if (tEntry == tMin[2]) hitAxis = 2;

	if (hitAxis != -1) {
		if (front[hitAxis] > 0) {
			info.direction = (hitAxis == 0 ? 3 : hitAxis == 1 ? 1 : 5);
		} else {
			info.direction = (hitAxis == 0 ? 2 : hitAxis == 1 ? 0 : 4);
		}
	}

	info.intersected = true;
	info.distance = glm::distance(camPos, camPos + tEntry * front);

	return info;
}