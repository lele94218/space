#include "material.h"

Material::Material() : id(unique_id_++) {}

int Material::unique_id_ = 0;
