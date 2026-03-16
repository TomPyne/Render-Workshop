#include "SphereBuilder.h"

#include "Logging/Logging.h"

namespace SphereBuilder
{
	SphereMesh_s SphereBuilder::BuildSphereMesh(const SphereMeshDesc_s& Desc)
	{
		SphereMesh_s Mesh = {};

		if(Desc.Type >= SphereType_e::COUNT)
		{
			LOGERROR("Invalid sphere type %d,", static_cast<int>(Desc.Type));
			return Mesh;
		}

		if(Desc.LatitudeSegments < 2)
		{
			LOGERROR("Latitude segments must be at least 2");
			return Mesh;
		}

		if(Desc.LongitudeSegments < 3)
		{
			LOGERROR("Longitude segments must be at least 3");
			return Mesh;
		}

		Mesh.Positions.reserve(Desc.LongitudeSegments * (Desc.LatitudeSegments - 1) + 2);

		Mesh.Positions.emplace_back(0.0f, Desc.Radius, 0.0f);

		for(uint32_t LatIt = 0; LatIt < Desc.LatitudeSegments - 1; LatIt++)
		{
			const float Phi = static_cast<float>(LatIt + 1) / Desc.LatitudeSegments * K_PI;
			const float SinPhi = sinf(Phi);
			const float CosPhi = cosf(Phi);
			for (uint32_t LongIt = 0; LongIt < Desc.LongitudeSegments; LongIt++)
			{
				const float Theta = static_cast<float>(LongIt) / Desc.LongitudeSegments * 2.0f * K_PI;
				const float SinTheta = sinf(Theta);
				const float CosTheta = cosf(Theta);

				Mesh.Positions.emplace_back(Desc.Radius * SinPhi * CosTheta, Desc.Radius * CosPhi, Desc.Radius * SinPhi * SinTheta);
			}
		}

		Mesh.Positions.emplace_back(0.0f, -Desc.Radius, 0.0f);
		const uint32_t SouthPoleIndex = static_cast<uint32_t>(Mesh.Positions.size() - 1);

		Mesh.Indices.reserve((Desc.LongitudeSegments * 6) + (Desc.LatitudeSegments - 2) * (Desc.LongitudeSegments * 6));

		for(uint32_t LongIt = 0; LongIt < Desc.LongitudeSegments; LongIt++)
		{
			Mesh.Indices.push_back(0);
			Mesh.Indices.push_back((LongIt + 1) % Desc.LongitudeSegments + 1);
			Mesh.Indices.push_back(LongIt + 1);

			Mesh.Indices.push_back(1);
			Mesh.Indices.push_back((LongIt + Desc.LongitudeSegments) * (Desc.LatitudeSegments - 2) + 1);
			Mesh.Indices.push_back((LongIt + 1) % Desc.LongitudeSegments + Desc.LongitudeSegments * (Desc.LatitudeSegments - 2) + 1);
		}

		for (uint32_t LatIt = 0; LatIt < Desc.LatitudeSegments - 2; LatIt++)
		{
			uint32_t Index0 = LatIt * Desc.LongitudeSegments + 1;
			uint32_t Index1 = (LatIt + 1) * Desc.LongitudeSegments + 1;
			for (uint32_t LongIt = 0; LongIt < Desc.LongitudeSegments; LongIt++)
			{
				Mesh.Indices.push_back(Index0 + LongIt);
				Mesh.Indices.push_back(Index0 + (LongIt + 1) % Desc.LongitudeSegments);
				Mesh.Indices.push_back(Index1 + (LongIt + 1) % Desc.LongitudeSegments);
				Mesh.Indices.push_back(Index1 + LongIt);
			}
		}

		return Mesh;
	}

}
