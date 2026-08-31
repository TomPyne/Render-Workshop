#pragma once

class IRenderable_c
{
public:

	virtual ~IRenderable_c() = default;

	virtual void Render(struct SpatialRenderingCollector_s& Collector) = 0;

};