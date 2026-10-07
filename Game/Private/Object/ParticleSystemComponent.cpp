#include "Object/ParticleSystemComponent.h"

#include "Particles/ParticleRenderer.h"
#include "Space/Space.h"

// Begin ObjectComponent_c interface
void ParticleSystemComponent_c::OnCreate()
{
	Super::OnCreate();

	if (Space_c* Space = GetSpace())
	{
		ParticleSystemInfo = new ParticleSystemInfo_s;
		ParticleSystemInfo->Name = "Test";
		ParticleSystemInfo->Position = GetWorldPosition();
		ParticleSystemInfo->Scale = 0.1f;
		Space->RegisterParticleSystem(ParticleSystemInfo);
	}

}

void ParticleSystemComponent_c::Deserialize(const struct JsonValue_s& Data)
{
	Super::Deserialize(Data);
}

void ParticleSystemComponent_c::PreDestroy()
{
	if (ParticleSystemInfo)
	{
		if (Space_c* Space = GetSpace())
		{
			Space->UnregisterParticleSystem(ParticleSystemInfo);
		}
		delete ParticleSystemInfo;
		ParticleSystemInfo = nullptr;
	}

	Super::PreDestroy();
}
// End ObjectComponent_c interface