#include "Texture.h"

#include "Renderer.h"

Texture::Texture(const std::string& pFilepath, const int pSlot)
	: rendererID(0), filepath(pFilepath), localBuffer(nullptr), width(0), height(0), bpp(0)
{
	stbi_set_flip_vertically_on_load(GL_TRUE);
	localBuffer = stbi_load(filepath.c_str(), &width, &height, &bpp, 0);

	if (!localBuffer) {
		std::cerr << "[ERROR] Failed to load texture: " << filepath << std::endl;
		return;
	}

	//GLCall(glCreateTextures(GL_TEXTURE_2D, 1, &rendererID));

	GLCall(glGenTextures(1, &rendererID));
	GLCall(glBindTexture(GL_TEXTURE_2D, rendererID));
	//GLCall(glActiveTexture(GL_TEXTURE0 + pSlot));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, localBuffer));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

	//GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	
	if (localBuffer) {
		std::cout << "Texture loaded: " << filepath << ", " << width << "x" << height << ", bpp: " << bpp << std::endl;
		stbi_image_free(localBuffer);
	} else {
		std::cout << "[ERROR] Failed to load texture" << std::endl;
		std::cout << stbi_failure_reason() << std::endl;
	}

	if (rendererID == 0 || width == 0 || height == 0) {
		std::cerr << "[ERROR] Failed to load texture or invalid dimensions" << std::endl;
	}
}

Texture::~Texture() {
	glDeleteTextures(1, &rendererID);
}

void Texture::bind(unsigned int slot /* = 0*/) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, rendererID);
}

void Texture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}