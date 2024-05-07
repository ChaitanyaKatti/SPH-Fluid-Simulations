#pragma once

#include <string>


typedef enum
{
    DIFFUSE,
    SPECULAR,
    NORMAL
} TextureType;

class Texture
{
private:
    int GL_TEXTURE_IDX;
    unsigned int ID;
    std::string path;
    TextureType type;
public:
    Texture(int GL_TEXTURE_IDX, const char* path, TextureType type);
    void Bind();
    void ActiveTexture();
};