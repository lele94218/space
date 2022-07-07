#include "geometry.h"

Geometry::Geometry() : id(unique_id_++) {}

int Geometry::unique_id_ = 0;
