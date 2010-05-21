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
#include "nel/misc/object_arena_allocator.h"
#include "nel/misc/fixed_size_allocator.h"

namespace NLMISC
{


CObjectArenaAllocator *CObjectArenaAllocator::_DefaultAllocator = NULL;


//*****************************************************************************************************************
CObjectArenaAllocator::CObjectArenaAllocator(uint maxAllocSize, uint granularity /* = 4*/)
{
	nlassert(granularity > 0);
	nlassert(maxAllocSize > 0);
	_MaxAllocSize = granularity * ((maxAllocSize + (granularity - 1)) / granularity);
	_ObjectSizeToAllocator.resize(_MaxAllocSize / granularity, NULL);
	_Granularity = granularity;
	#ifdef NL_DEBUG
		_AllocID = 0;
		_WantBreakOnAlloc = false;
		_BreakAllocID = 0;
	#endif
}

//*****************************************************************************************************************
CObjectArenaAllocator::~CObjectArenaAllocator()
{
	for(uint k = 0; k < _ObjectSizeToAllocator.size(); ++k)
	{
		delete _ObjectSizeToAllocator[k];
	}
}

//*****************************************************************************************************************
void *CObjectArenaAllocator::alloc(uint size)
{
	#ifdef NL_DEBUG
		if (_WantBreakOnAlloc)
		{
			if (_AllocID == _BreakAllocID)
			{
				nlassert(0);
			}
		}
	#endif
	if (size >= _MaxAllocSize)
	{
		// use standard allocator
		uint8 *block = new uint8[size + sizeof(uint)]; // an additionnal uint is needed to store size of block
		if (!block) return NULL;
		#ifdef NL_DEBUG
			_MemBlockToAllocID[block] = _AllocID;
		#endif
		*(uint *) block = size;
		return block + sizeof(uint);
	}
	uint entry = ((size + (_Granularity - 1)) / _Granularity) ;
	nlassert(entry < _ObjectSizeToAllocator.size());
	if (!_ObjectSizeToAllocator[entry])
	{
		_ObjectSizeToAllocator[entry] = new CFixedSizeAllocator(entry * _Granularity + sizeof(uint), _MaxAllocSize / size); // an additionnal uint is needed to store size of block
	}
	void *block = _ObjectSizeToAllocator[entry]->alloc();
	#ifdef NL_DEBUG
		if (block)
		{
			_MemBlockToAllocID[block] = _AllocID;
		}
		++_AllocID;
	#endif
	*(uint *) block = size;
	return (void *) ((uint8 *) block + sizeof(uint));
}

//*****************************************************************************************************************
void CObjectArenaAllocator::free(void *block)
{
	if (!block) return;
	uint8 *realBlock = (uint8 *) block - sizeof(uint); // a uint is used at start of block to give its size
	uint size = *(uint *) realBlock;
	if (size >= _MaxAllocSize)
	{
		#ifdef NL_DEBUG
				std::map<void *, uint>::iterator it = _MemBlockToAllocID.find(realBlock);
				nlassert(it != _MemBlockToAllocID.end());
				_MemBlockToAllocID.erase(it);
		#endif
		delete realBlock;
		return;
	}
	uint entry = ((size + (_Granularity - 1)) / _Granularity);
	nlassert(entry < _ObjectSizeToAllocator.size());
	_ObjectSizeToAllocator[entry]->free(realBlock);
	#ifdef NL_DEBUG
		std::map<void *, uint>::iterator it = _MemBlockToAllocID.find(realBlock);
		nlassert(it != _MemBlockToAllocID.end());
		/*
		#ifdef NL_DEBUG
				if (_WantBreakOnAlloc)
				{
					if (it->second == _BreakAllocID)
					{
						nlassert(0);
					}
				}
		#endif
		*/
		_MemBlockToAllocID.erase(it);
	#endif
}

//*****************************************************************************************************************
uint CObjectArenaAllocator::getNumAllocatedBlocks() const
{
	uint numObjs = 0;
	for(uint k = 0; k < _ObjectSizeToAllocator.size(); ++k)
	{
		if (_ObjectSizeToAllocator[k]) numObjs += _ObjectSizeToAllocator[k]->getNumAllocatedBlocks();
	}
	return numObjs;
}

//*****************************************************************************************************************
CObjectArenaAllocator &CObjectArenaAllocator::getDefaultAllocator()
{
	if (!_DefaultAllocator)
	{
		_DefaultAllocator = new CObjectArenaAllocator(32768);
	}
	return *_DefaultAllocator;
}


#ifdef NL_DEBUG

//*****************************************************************************************************************
void CObjectArenaAllocator::dumpUnreleasedBlocks()
{
	for(std::map<void *, uint>::iterator it = _MemBlockToAllocID.begin(); it != _MemBlockToAllocID.end(); ++it)
	{
	  nlinfo("block %u at adress %p remains", it->second, (static_cast<uint8 *>(it->first) + sizeof(uint)));
	}
}

//*****************************************************************************************************************
void CObjectArenaAllocator::setBreakForAllocID(bool enabled, uint id)
{
	_WantBreakOnAlloc = enabled;
	_BreakAllocID = id;
}

#endif // NL_DEBUG


} // NLMISC

