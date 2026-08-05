#pragma once

struct JsonValue_s;
struct Material_s;
struct Path_s;
struct Shader_s;

#include <memory>

namespace MaterialManager
{
	std::shared_ptr<Material_s> RequestMaterial(const Path_s& Path);
	std::shared_ptr<Material_s> RequestMaterial(const JsonValue_s& Data);

	std::shared_ptr<Shader_s> RequestShader(const Path_s& Path);
	std::shared_ptr<Shader_s> RequestShader(const JsonValue_s& Data);
}