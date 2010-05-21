// NeL - MMORPG Framework <http://dev.ryzom.com/projects/nel/>
// Copyright (C) 2010  Winch Gate Property Limited
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "stdmisc.h"

#include <ctime>

#ifdef NL_OS_WINDOWS
#	define NOMINMAX
#	include <windows.h>
#elif defined (NL_OS_UNIX)
#	include <sys/time.h>
#	include <unistd.h>
#endif

#ifdef NL_OS_MAC
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "nel/misc/time_nl.h"
#include "nel/misc/sstring.h"

namespace NLMISC
{

/* Return the number of second since midnight (00:00:00), January 1, 1970,
 * coordinated universal time, according to the system clock.
 * This values is the same on all computer if computers are synchronized (with NTP for example).
 */
uint32 CTime::getSecondsSince1970 ()
{
	return uint32(time(NULL));
}

/** Return the number of second since midnight (00:00:00), January 1, 1970,
 * coordinated universal time, according to the system clock.
 * The time returned is UTC (aka GMT+0), ie it does not have the local time ajustement
 * nor it have the daylight saving ajustement.
 * This values is the same on all computer if computers are synchronized (with NTP for example).
 */
//uint32	CTime::getSecondsSince1970UTC ()
//{
//	// get the local time
//	time_t nowLocal = time(NULL);
//	// convert it to GMT time (UTC)
//	struct tm * timeinfo;
//	timeinfo = gmtime(&nowLocal);
//	return nl_mktime(timeinfo);
//}

/* Return the local time in milliseconds.
 * Use it only to measure time difference, the absolute value does not mean anything.
 * On Unix, getLocalTime() will try to use the Monotonic Clock if available, otherwise
 * the value can jump backwards if the system time is changed by a user or a NTP time sync process.
 * The value is different on 2 different computers; use the CUniTime class to get a universal
 * time that is the same on all computers.
 * \warning On Win32, the value is on 32 bits only. It wraps around to 0 every about 49.71 days.
 */
TTime CTime::getLocalTime ()
{

#ifdef NL_OS_WINDOWS

	//static bool initdone = false;
	//static bool byperfcounter;
	// Initialization
	//if ( ! initdone )
	//{
		//byperfcounter = (getPerformanceTime() != 0);
		//initdone = true;
	//}

	/* Retrieve time is ms
     * Why do we prefer getPerformanceTime() to timeGetTime() ? Because on one dual-processor Win2k
	 * PC, we have noticed that timeGetTime() slows down when the client is running !!!
	 */
	/* Now we have noticed that on all WinNT4 PC the getPerformanceTime can give us value that
	 * are less than previous
	 */

	//if ( byperfcounter )
	//{
	//	return (TTime)(ticksToSecond(getPerformanceTime()) * 1000.0f);
	//}
	//else
	//{
		// This is not affected by system time changes. But it cycles every 49 days.
		return timeGetTime();
	//}

#elif defined (NL_OS_UNIX)

	static bool initdone = false;
	static bool isMonotonicClockSupported = false;
	if ( ! initdone )
	{

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
#if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK >= 0)

		/* Initialize the local time engine.
		* On Unix, this method will find out if the Monotonic Clock is supported
		* (seems supported by kernel 2.6, not by kernel 2.4). See getLocalTime().
		*/
		struct timespec tv;
		if ( (clock_gettime( CLOCK_MONOTONIC, &tv ) == 0) &&
			 (clock_getres( CLOCK_MONOTONIC, &tv ) == 0) )
		{
//			nldebug( "Monotonic local time supported (resolution %.6f ms)", ((float)tv.tv_sec)*1000.0f + ((float)tv.tv_nsec)/1000000.0f );
			isMonotonicClockSupported = true;
		}
		else

#endif
#endif
		{
//			nlwarning( "Monotonic local time not supported, caution with time sync" );
		}

		initdone = true;
	}

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
#if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK >= 0)

	if ( isMonotonicClockSupported )
	{
		struct timespec tv;
		// This is not affected by system time changes.
		if ( clock_gettime( CLOCK_MONOTONIC, &tv ) != 0 )
			nlerror ("Can't get clock time again");
	    return (TTime)tv.tv_sec * (TTime)1000 + (TTime)((tv.tv_nsec/*+500*/) / 1000000);
	}

#endif
#endif

	// This is affected by system time changes.
	struct timeval tv;
	if ( gettimeofday( &tv, NULL) != 0 )
		nlerror ("Can't get time of day");
	return (TTime)tv.tv_sec * (TTime)1000 + (TTime)tv.tv_usec / (TTime)1000;

#endif
}


/* Return the time in processor ticks. Use it for profile purpose.
 * If the performance time is not supported on this hardware, it returns 0.
 * \warning On a multiprocessor system, the value returned by each processor may
 * be different. The only way to workaround this is to set a processor affinity
 * to the measured thread.
 * \warning The speed of tick increase can vary (especially on laptops or CPUs with
 * power management), so profiling several times and computing the average could be
 * a wise choice.
 */
TTicks CTime::getPerformanceTime ()
{
#ifdef NL_OS_WINDOWS
	LARGE_INTEGER ret;
	if (QueryPerformanceCounter (&ret))
		return ret.QuadPart;
	else
		return 0;
#elif defined(NL_OS_MAC)
	return mach_absolute_time();
#else
#if defined(HAVE_X86_64)
	unsigned long long int hi, lo;
	__asm__ volatile (".byte 0x0f, 0x31" : "=a" (lo), "=d" (hi));
	return (hi << 32) | (lo & 0xffffffff);
#elif defined(HAVE_X86) and !defined(NL_OS_MAC)
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
#else // HAVE_X86
	static bool firstWarn = true;
	if (firstWarn)
	{
		nlwarning ("TTicks CTime::getPerformanceTime () is not implemented for your processor, returning 0");
		firstWarn = false;
	}
	return 0;
#endif // HAVE_X86

#endif // NL_OS_WINDOWS
}
/*
#define GETTICKS(t) asm volatile ("push %%esi\n\t" "mov %0, %%esi" : : "r" (t)); \
                      asm volatile ("push %eax\n\t" "push %edx"); \
                      asm volatile ("rdtsc"); \
                      asm volatile ("movl %eax, (%esi)\n\t" "movl %edx, 4(%esi)"); \
                      asm volatile ("pop %edx\n\t" "pop %eax\n\t" "pop %esi");
*/


/* Convert a ticks count into second. If the performance time is not supported on this
 * hardware, it returns 0.0.
 */
double CTime::ticksToSecond (TTicks ticks)
{
#ifdef NL_OS_WINDOWS
	LARGE_INTEGER ret;
	if (QueryPerformanceFrequency(&ret))
	{
		return (double)(sint64)ticks/(double)ret.QuadPart;
	}
	else
#elif defined(NL_OS_MAC)
	{
		static double factor = 0.0;
		if (factor == 0.0)
		{
			mach_timebase_info_data_t tbInfo;
			mach_timebase_info(&tbInfo);
			factor = 1000000000.0 * (double)tbInfo.numer / (double)tbInfo.denom;
		}
		return double(ticks / factor);
	}
#endif // NL_OS_WINDOWS
	{
		static bool benchFrequency = true;
		static sint64 freq = 0;
		if (benchFrequency)
		{
			// try to have an estimation of the cpu frequency

			TTicks tickBefore = getPerformanceTime ();
			TTicks tickAfter = tickBefore;
			TTime timeBefore = getLocalTime ();
			TTime timeAfter = timeBefore;
			for(;;)
			{
				if (timeAfter - timeBefore > 1000)
					break;
				timeAfter = getLocalTime ();
				tickAfter = getPerformanceTime ();
			}

			TTime timeDelta = timeAfter - timeBefore;
			TTicks tickDelta = tickAfter - tickBefore;

			freq = 1000 * tickDelta / timeDelta;
			benchFrequency = false;
		}

		return (double)(sint64)ticks/(double)freq;
	}
}


std::string	CTime::getHumanRelativeTime(sint32 nbSeconds)
{
	sint32 delta = nbSeconds;
	if (delta < 0)
		delta = -delta;

	// some constants of time duration in seconds
	const sint32 oneMinute = 60;
	const sint32 oneHour = oneMinute * 60;
	const sint32 oneDay = oneHour * 24;
	const sint32 oneWeek = oneDay * 7;
	const sint32 oneMonth = oneDay * 30; // aprox, a more precise value is 30.416666... but no matter
	const sint32 oneYear = oneDay * 365; // aprox, a more precise value is 365.26.. who care?

	sint32 year, month, week, day, hour, minute;
	year = month = week = day = hour = minute = 0;

	/// compute the different parts
	year = delta / oneYear;
	delta %= oneYear;

	month = delta / oneMonth;
	delta %= oneMonth;

	week = delta / oneWeek;
	delta %= oneWeek;

	day = delta / oneDay;
	delta %= oneDay;

	hour = delta / oneHour;
	delta %= oneHour;

	minute = delta / oneMinute;
	delta %= oneMinute;

	// compute the string
	CSString ret;

	if (year)
		ret << year << " years ";
	if (month)
		ret << month << " months ";
	if (week)
		ret << week << " weeks ";
	if (day)
		ret << day << " days ";
	if (hour)
		ret << hour << " hours ";
	if (minute)
		ret << minute << " minutes ";
	if (delta || ret.empty())
		ret << delta << " seconds ";

	return ret;
}

#ifdef NL_OS_WINDOWS
	/** Return the offset in 10th of micro sec between the windows base time (
	 *	01-01-1601 0:0:0 UTC) and the unix base time (01-01-1970 0:0:0 UTC).
	 *	This value is used to convert windows system and file time back and
	 *	forth to unix time (aka epoch)
	 */
	uint64 CTime::getWindowsToUnixBaseTimeOffset()
	{
		static bool init = false;

		static uint64 offset = 0;

		if (! init)
		{
			// compute the offset to convert windows base time into unix time (aka epoch)
			// build a WIN32 system time for jan 1, 1970
			SYSTEMTIME baseTime;
			baseTime.wYear = 1970;
			baseTime.wMonth = 1;
			baseTime.wDayOfWeek = 0;
			baseTime.wDay = 1;
			baseTime.wHour = 0;
			baseTime.wMinute = 0;
			baseTime.wSecond = 0;
			baseTime.wMilliseconds = 0;

			FILETIME baseFileTime = {0,0};
			// convert it into a FILETIME value
			SystemTimeToFileTime(&baseTime, &baseFileTime);
			offset = baseFileTime.dwLowDateTime | (uint64(baseFileTime.dwHighDateTime)<<32);

			init = true;
		}

		return offset;
	}
#endif


} // NLMISC
