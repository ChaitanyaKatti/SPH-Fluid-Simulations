#pragma once

#include <glm/glm.hpp>

glm::vec3 getRandVec3();

void genRandVec3Array(glm::vec3 *arr, int n, float scale);

void genUniformVec3Array(glm::vec3 *arr, int n, float scale);

glm::vec3 colorParticleBWR(float speed);
glm::vec3 colorParticleBGR1(float speed);
glm::vec3 colorParticleBGR2(float speed);