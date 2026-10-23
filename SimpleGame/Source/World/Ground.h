#pragma once

// The only source of ground height, so terrain only has to change this.
inline float GetGroundHeight(float /*X*/, float /*Z*/)
{
	return 0.0f;
}
