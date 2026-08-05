#include "Assets/MaterialManager.h"

#include "Rendering/Materials.h"

#include <Render/RenderTypes.h>

namespace MaterialManager
{

std::shared_ptr<Material_s> RequestMaterial(const Path_s& Path)
{
    return std::shared_ptr<Material_s>();
}

std::shared_ptr<Material_s> RequestMaterial(const JsonValue_s& Data)
{
    return nullptr;
}

std::shared_ptr<Shader_s> RequestShader(const Path_s& Path)
{
    return std::shared_ptr<Shader_s>();
}

std::shared_ptr<Shader_s> RequestShader(const JsonValue_s& Data)
{
    return nullptr;
}

}