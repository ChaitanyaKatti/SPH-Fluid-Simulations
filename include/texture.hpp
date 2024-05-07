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
    unsigned int ID;
    TextureType type;
    std::string path;
public:
    Texture(unsigned int ID, const char* path, TextureType type);
    void Bind();
};