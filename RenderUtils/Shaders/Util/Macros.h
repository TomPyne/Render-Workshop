#pragma once

#define MAKE_CBV_SLOT_HELPER(Slot) b##Slot
#define MAKE_CBV_SLOT(Slot) MAKE_CBV_SLOT_HELPER(Slot)