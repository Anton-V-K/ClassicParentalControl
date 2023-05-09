#pragma once

#include <Common/REPEAT.h>

#define UPDATE_CHILDWINDOW(nID) \
			{ nID,  UPDUI_CHILDWINDOW },

#define UPDATE_CHILDWINDOWS(nStartID, nEndId, count) \
    REPEAT_N(UPDATE_CHILDWINDOW, count, nStartID)
