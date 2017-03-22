
#include "event_q.h"
#include "stream.h"

lightblue_evt_t event_get()
{
	if (!stream_q_empty())
	{
		return EVENT_TYPE_STREAM;
	}

	return EVENT_TYPE_NONE;
}