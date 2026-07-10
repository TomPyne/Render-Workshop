#include "SpaceRenderer.h"

#include "Game/Object/ObjectComponent.h"
#include "Game/Rendering/IRenderable.h"
#include "Game/Space/Space.h"

void SpaceRenderer_c::RenderSpace(Space_c* Space, rl::CommandListSubmissionGroup& clGroup)
{
	if (!Space)
		return;

	SpatialRenderingCollector_s Collector = {};

	for (std::shared_ptr<Object_c>& Object : Space->Objects)
	{
		for (std::shared_ptr<ObjectComponent_c>& Component : Object->Components)
		{
			if (IRenderable_c* Renderable = dynamic_cast<IRenderable_c*>(Component.get()))
			{
				Renderable->Render(Collector);
			}
		}
	}


}
