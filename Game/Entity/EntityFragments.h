#pragma once

#include "Entity.h"

#include <cmath>
#include <vector>

struct EntityBitField_s
{
	EntityBitField_s() = default;

	EntityBitField_s(uint32_t PageCount)
		: Pages(PageCount, 0)
	{}

	std::vector<uint64_t> Pages;

	void SetBit(uint32_t Index, bool Value) noexcept
	{
		const uint32_t PageIndex = Index / 64;
		if (Pages.size() <= PageIndex)
		{
			Pages.resize(PageIndex + 1);
		}
		const uint32_t BitIndex = Index % 64;
		if (Value)
		{
			Pages[PageIndex] |= (1ull << BitIndex);
		}
		else
		{
			Pages[PageIndex] &= ~(1ull << BitIndex);
		}
	}

	bool GetBit(uint32_t Index) const noexcept
	{
		const uint32_t PageIndex = Index / 64;
		if (Pages.size() > PageIndex)
		{			
			const uint32_t BitIndex = Index % 64;
			return (Pages[PageIndex] & (1ull << BitIndex)) != 0;
		}
		return false;
	}

	inline EntityBitField_s& operator&(const EntityBitField_s& Other)
	{
		if (Other.Pages.size() > Pages.size())
		{
			Pages.resize(Other.Pages.size(), 0);
		}

		for (size_t i = 0; i < Pages.size(); i++)
		{
			Pages[i] = Pages[i] & Other.Pages[i];
		}

		return *this;
	}
};

struct EntityFragmentEntry_s
{
	uint32_t FragmentIndex;
	uint32_t FragmentID;
};

using EntityFragmentEntries_t = std::vector<EntityFragmentEntry_s>;

template<typename FragmentType>
struct EntityFragmentRegister_s
{
	std::vector<EntityFragmentEntries_t> EntityIDs; // Indexed by Entity_t
	std::vector<FragmentType> Fragments;
	std::vector<uint32_t> FreeList;
	EntityBitField_s ActiveFragments;

	enum class Fragment_t : uint32_t { INVALID };

	Fragment_t LastFragmentID = Fragment_t::INVALID;

	static EntityFragmentRegister_s Get()
	{
		static EntityFragmentRegister_s Instance;
		return Instance;
	}

	const EntityBitField_s& GetFragmentBitField() const noexcept
	{
		return ActiveFragments;
	}

	Fragment_t CreateFragment(Entity_t Entity, const FragmentType& Fragment)
	{
		Fragment_t FragmentID = ++LastFragmentID;

		if (EntityIDs.size() <= (uint32_t)Entity)
		{
			EntityIDs.resize((uint32_t)Entity + 1);
		}

		uint32_t FragmentIndex = 0;
		if (FreeList.empty())
		{
			FragmentIndex = Fragment.size();
			Fragments.resize(Fragments.size() + 1);
		}
		else
		{
			FragmentIndex = (Fragment_t)FreeList.back();
			FreeList.pop_back();
		}

		Fragments[FragmentIndex] = Fragment;
		ActiveFragments.SetBit(FragmentIndex, true);

		EntityFragmentEntry_s NewEntry = { FragmentIndex, FragmentID };

		EntityFragmentEntries_t& Entries = EntityIDs[(uint32_t)Entity];
		Entries.push_back(NewEntry);

		return FragmentID;
	}

	void DestroyFragment(Entity_t Entity, Fragment_t FragmentID)
	{
		if (EntityIDs.size() <= (uint32_t)Entity)
			return;

		EntityFragmentEntries_t& Entries = EntityIDs[(uint32_t)Entity];
		for (size_t i = 0; i < Entries.size(); i++)
		{
			if (Entries[i].FragmentID == (uint32_t)FragmentID)
			{
				uint32_t FragmentIndex = Entries[i].FragmentIndex;
				FreeList.push_back(FragmentIndex);
				ActiveFragments.SetBit(FragmentIndex, false);

				std::swap(Entries[i], Entries.back());
				Entries.pop_back();

				return;
			}
		}
	}

	void NumFragments() const noexcept
	{
		return Fragments.size();
	}

	void FragmentValid(uint32_t Index) const noexcept
	{
		return ActiveFragments.GetBit(Index);
	}

	template<typename F>
	void ForEachFragment(F&& Func)
	{
		const size_t FragmentCount = Fragments.size();
		size_t FragIt = 0;
		uint64_t Page = 0;
		size_t PageIt = 0;
		size_t CurrentPageIt = 0;
		for (; FragIt < FragmentCount; ++FragIt, ++CurrentPageIt)
		{
			if (CurrentPageIt >= 64)
			{
				PageIt++;
				if (ActiveFragments.Pages.size() <= PageIt)
				{
					break; // Ensure?
				}
				Page = ActiveFragments.Pages[PageIt];
				CurrentPageIt = 0;
			}

			if ((Page & (1ull << CurrentPageIt)) != 0)
			{
				if(!Func(Fragments[FragIt]))
				{
					break;
				}
			}
		}
	}
};

template<typename FragmentType>
FragmentType* GetFirstFragment(Entity_t* OptEntity = nullptr)
{
	EntityFragmentRegister_s::Get().ForEachFragment([&](FragmentType& Fragment)
	{
		if (OptEntity)
		{
			// Find entity for fragment
		}
		return false; // Stop after first
	});
}

template<typename... FragmentTypes>
Entity_t GetEntityWithFragments()
{
	EntityBitField_s RequiredFragmentsBitField;
	((RequiredFragmentsBitField &= EntityFragmentRegister_s<FragmentTypes>::Get().GetBitField()), ...);
}