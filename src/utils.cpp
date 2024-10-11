#include <glm/glm.hpp>
#include <utils.hpp>

glm::vec3 getRandVec3()
{
    return glm::vec3(
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

void genRandVec3Array(glm::vec3 *arr, int n, float scale = 1.0f)
{
    for (int i = 0; i < n; i++)
    {
        arr[i] = getRandVec3() * scale;
    }
}

void genUniformVec3Array(glm::vec3 *arr, int n, float scale = 1.0f)
{
    if (n <= 0)
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                arr[i * n * n + j * n + k] = glm::vec3(i, j, k) * (scale / (n - 1)); //+ getRandVec3() * 0.01f;
            }
        }
    }
    // std::cout << "Uniform array created of size: " << (float)sizeof(glm::vec3) * n * n * n / 1024 / 1024 << " Mbs" << std::endl;
}

glm::vec3 colorParticleBWR(float t)
{
    // Blue to White to Red, from t=0 to t=1
    float R = (0.5 + t - abs(t - 0.5));
    float G = 4 * (1 - t) * t;
    float B = (1.5 - t - abs(t - 0.5));
    R = R * R;
    B = B * B;
    return glm::vec3(R, G, B);
}

glm::vec3 colorParticleBGR1(float t)
{
    // Blue to Green to Red, from t=0 to t=1
    float R = 0.8 * t + 0.2;
    float G = 0.8 * 4 * (1 - t) * t + 0.2;
    float B = 0.8 * (1 - t) + 0.2;
    return glm::vec3(R, G, B);
}

glm::vec3 colorParticleBGR2(float t)
{
    // Blue to Green to Red, from t=0 to t=1
    float R = abs(t - 0.5) + t - 0.5;
    float G = 1 - 2 * abs(t - 0.5);
    float B = abs(t - 0.5) - t + 0.5;
    return glm::vec3(R, G, B);
}