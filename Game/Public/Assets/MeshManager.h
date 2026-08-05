#pragma once

#include <memory>

struct Mesh_s;
struct JsonValue_s;

namespace MeshManager
{

std::shared_ptr<Mesh_s> RequestMesh(const JsonValue_s& Data);

}