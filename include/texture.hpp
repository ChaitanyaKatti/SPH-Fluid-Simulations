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
public:
    virtual void Bind() = 0;
};

class ImageTexture: public Texture
{
private:
    int GL_TEXTURE_IDX;
    unsigned int ID;
    std::string path;
    TextureType type;
public:
    ImageTexture(int GL_TEXTURE_IDX, const char* path, TextureType type);
    void Bind();
};

class RenderTexture: public Texture
{
private:
    unsigned int ID;
    unsigned int FBO;
    unsigned int RBO;
    int width;
    int height;
public:
    RenderTexture(int width, int height);
    void Bind();
    void MakeRenderTarget();
};

class ColorDepthTexture: public Texture
{
private:
    unsigned int FBO;
    unsigned int colorTexture;
    unsigned int depthTexture;
    int width;
    int height;
public:
    ColorDepthTexture(int width, int height);
    void Bind();
    void MakeRenderTarget();
};