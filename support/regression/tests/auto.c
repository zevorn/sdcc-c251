/*
   C2X auto
*/

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c23

char c = 7;
auto i = 7;

#endif

void
testAuto(void)
{
#ifdef __SDCC
	struct auto_record
	{
		int value;
	};
	int k = 11;
	static auto s = 7;
	auto j = c + 3;
	auto p = &k;
	auto record = (struct auto_record){13};
	ASSERT (sizeof (s) == sizeof (int));
	ASSERT (sizeof (i) == sizeof (int));
	ASSERT (sizeof (j) == sizeof (int));
	ASSERT (*p == 11);
	ASSERT (record.value == 13);
#endif
}
