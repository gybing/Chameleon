/*
  Chameleon, a UCI chinese chess playing engine derived from Stockfish
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2017 Wilbert Lee

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <Windows.h>

#include "misc.h"
#include "thread_win32.h"

using namespace std;

// Debug functions used mainly to collect run-time statistics
static int64_t hits[2], means[2];

// engine_info() returns the full name of the current Stockfish version. This
// will be either "Stockfish <Tag> DD-MM-YY" (where DD-MM-YY is the date when
// the program was compiled) or "Stockfish <Version>", depending on whether
// Version is empty.
const string engine_info(bool to_uci)
{
	stringstream s; // From compiler, format is "Sep 21 2008"

	if (to_uci)
	{
		s << "id name SF2C-ES" << "\n";
		s << "id author Wilbert Lee" << "\n";
	}
	else
	{
		s << "uciok" << "\n";
		s << "info initend" << "\n";
	}
	return s.str();
}

void dbg_hit_on(bool b)
{
	++hits[0];
	if (b)
		++hits[1];
}

void dbg_hit_on(bool c, bool b)
{
	if (c) dbg_hit_on(b);
}

void dbg_mean_of(int v)
{
	++means[0];
	means[1] += v;
}

void dbg_print()
{
	if (hits[0])
		cerr << "Total " << hits[0] << " Hits " << hits[1]
		<< " hit rate (%) " << 100 * hits[1] / hits[0] << endl;

	if (means[0])
		cerr << "Total " << means[0] << " Mean "
		<< (double)means[1] / means[0] << endl;
}

// Used to serialize access to std::cout to avoid multiple threads writing at
// the same time.
std::ostream& operator<<(std::ostream& os, SyncCout sc)
{
	static Mutex m;

	if (sc == IO_LOCK)
		m.lock();

	if (sc == IO_UNLOCK)
		m.unlock();

	return os;
}

// prefetch() preloads the given address in L1/L2 cache. This is a non-blocking
// function that doesn't stall the CPU waiting for data to be loaded from memory,
// which can be quite slow.
#ifdef NO_PREFETCH

void prefetch(void*) {}

#else

void prefetch(void* addr)
{
#  if defined(__INTEL_COMPILER)
	// This hack prevents prefetches from being optimized away by
	// Intel compiler. Both MSVC and gcc seem not be affected by this.
	__asm__("");
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	_mm_prefetch((char*)addr, _MM_HINT_T0);
#  else
	__builtin_prefetch(addr);
#  endif
}

#endif