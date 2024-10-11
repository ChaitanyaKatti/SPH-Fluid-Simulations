#include <texture.hpp>

#include <glad/glad.h>
#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

#include <iostream>

Texture::Texture(int GL_TEXTURE_IDX, const char* path, TextureType type)
{
    this->GL_TEXTURE_IDX = GL_TEXTURE_IDX;
    this->type = type;
    this->path = path;


    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Flip the image
    stbi_set_flip_vertically_on_load(true);

    // load image, create texture and generate mipmaps
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            std::cout << "Error: Unknown number of components: " << nrComponents << std::endl;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }

    stbi_image_free(data);
}

void Texture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::ActiveTexture()
{
    glActiveTexture(GL_TEXTURE_IDX);
}