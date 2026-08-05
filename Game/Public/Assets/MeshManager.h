#pragma once

#include <memory>

struct JsonValue_s;
struct Mesh_s;
struct Path_s;

namespace MeshManager
{

std::shared_ptr<Mesh_s> RequestMesh(const Path_s& Path);
std::shared_ptr<Mesh_s> RequestMesh(const JsonValue_s& Data);

}