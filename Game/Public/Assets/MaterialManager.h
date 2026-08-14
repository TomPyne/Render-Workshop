#pragma once

class MaterialShader_c;
class MaterialShaderInstance_c;

struct JsonValue_s;
struct Path_s;
struct Shader_s;

#include <functional>
#include <memory>
#include <string>

namespace MaterialManager
{
	std::shared_ptr<MaterialShaderInstance_c> RequestMaterialInstance(const Path_s& Path);
	std::shared_ptr<MaterialShaderInstance_c> RequestMaterialInstance(const JsonValue_s& Data);

	struct MaterialShaderFactory_s
	{
		static MaterialShaderFactory_s& Get();

		std::unordered_map<std::wstring, std::function<std::shared_ptr<MaterialShader_c>()>> MaterialShaderFactoryCallbacks;

		void RegisterMaterialShaderClass(const std::wstring& ClassName, std::function<std::shared_ptr<MaterialShader_c>()>&& Func)
		{
			MaterialShaderFactoryCallbacks[ClassName] = Func;
		}

		std::shared_ptr<MaterialShader_c> CreateShaderMaterial(const std::wstring& ClassName);
	};

	template<class MaterialShaderType>
	void RegisterMaterialShaderClass(const std::wstring& ClassName)
	{
		MaterialShaderFactory_s::Get().RegisterMaterialShaderClass(ClassName, []() -> std::shared_ptr<MaterialShader_c>
		{
			return std::make_shared<MaterialShaderType>();
		});
	}
}