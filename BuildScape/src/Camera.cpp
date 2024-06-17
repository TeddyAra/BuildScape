#include "Camera.h"

#include "Input.h"
#include "glm/gtc/matrix_transform.hpp"

Camera::Camera(glm::vec3 pPos, glm::vec3 pFront, glm::vec3 pUp, float pSensitivity, float pFov, float pSpeed)
	: position(pPos), front(pFront), up(pUp), sensitivity(pSensitivity), fov(pFov), speed(pSpeed), locked(false)
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