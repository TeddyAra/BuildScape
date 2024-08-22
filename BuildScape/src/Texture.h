#pragma once

#include <iostream>
#include <string>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "vendor/stb_image/stb_image.h"

class Texture {
public:
	Texture(const std::string &pFilepath, const int pSlot);
	~Texture();

	void bind(unsigned int slot = 0) const;
	void unbind() const;

private:
	unsigned int rendererID;
	std::string filepath;
	unsigned char* localBuffer;

	int width;
	int height;
	int bpp;
};