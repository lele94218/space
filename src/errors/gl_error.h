#pragma once

#include <glad/glad.h>

GLenum GLCheckError_(const char* file, int line);

#define GLCheckError() GLCheckError_(__FILE__, __LINE__)
