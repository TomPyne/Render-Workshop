#pragma once

class IRenderable_c
{
public:

	virtual void Render(struct SpatialRenderingCollector_s& Collector) = 0;

};