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

#include "stdsound.h"
#include "nel/sound/stream_source.h"

// STL includes

// NeL includes
// #include <nel/misc/debug.h>
#include <nel/misc/vector.h>

// Project includes
#include "nel/sound/driver/buffer.h"
#include "nel/sound/audio_mixer_user.h"
#include "nel/sound/stream_sound.h"
#include "nel/sound/clustered_sound.h"

// using namespace std;
using namespace NLMISC;

namespace NLSOUND {

CStreamSource::CStreamSource(CStreamSound *streamSound, bool spawn, TSpawnEndCallback cb, void *cbUserParam, NL3D::CCluster *cluster)
	: CSourceCommon(streamSound, spawn, cb, cbUserParam, cluster), 
	m_StreamSound(streamSound), 
	m_Alpha(0.0f), 
	m_Track(NULL), 
	m_FreeBuffers(3),
	m_NextBuffer(0),
	m_LastSize(0),
	m_BytesPerSecond(0)
{
	nlassert(m_StreamSound != 0);

	// get a local copy of the stream sound parameter
	m_Alpha = m_StreamSound->getAlpha();//m_Buffers
	
	// create the three buffer objects
	CAudioMixerUser *mixer = CAudioMixerUser::instance();
	ISoundDriver *driver = mixer->getSoundDriver();
	m_Buffers[0] = driver->createBuffer();
	m_Buffers[1] = driver->createBuffer();
	m_Buffers[2] = driver->createBuffer();
}

CStreamSource::~CStreamSource()
{
	if (_Playing)
		stop();

	if (m_Buffers[0] != NULL) { delete m_Buffers[0]; m_Buffers[0] = NULL; }
	if (m_Buffers[1] != NULL) { delete m_Buffers[1]; m_Buffers[1] = NULL; }
	if (m_Buffers[2] != NULL) { delete m_Buffers[2]; m_Buffers[2] = NULL; }
}

void CStreamSource::initPhysicalSource()
{
	CAudioMixerUser *mixer = CAudioMixerUser::instance();
	CTrack *track = mixer->getFreeTrack(this);
	if (track != NULL)
	{
		nlassert(track->hasPhysicalSource());
		m_Track = track;
		getPhysicalSource()->setStreaming(true);
	}
}

void CStreamSource::releasePhysicalSource()
{
	if (hasPhysicalSource())
	{
		CAudioMixerUser *mixer = CAudioMixerUser::instance();
		ISource *pSource = getPhysicalSource();
		nlassert(pSource != NULL);

		// free the track
		pSource->stop();
		pSource->setStreaming(false);
		mixer->freeTrack(m_Track);
		m_Track = NULL;
	}
}

uint32 CStreamSource::getTime()
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);
	
	if (hasPhysicalSource())
		return getPhysicalSource()->getTime();
	else
		return 0;
}

bool CStreamSource::isPlaying()
{
	return _Playing;
}

/// Set looping on/off for future playbacks (default: off)
void CStreamSource::setLooping(bool l)
{
	//CAutoMutex<CMutex> autoMutex(m_BufferMutex);
	//
	//CSourceCommon::setLooping(l);
	//if (hasPhysicalSource())
	//	getPhysicalSource()->setLooping(l);
}

CVector CStreamSource::getVirtualPos() const
{
	if (getCluster() != 0)
	{
		// need to check the cluster status
		const CClusteredSound::CClusterSoundStatus *css = CAudioMixerUser::instance()->getClusteredSound()->getClusterSoundStatus(getCluster());
		if (css != 0)
		{
			// there is some data here, update the virtual position of the sound.
			float dist = (css->Position - getPos()).norm();
			CVector vpos(CAudioMixerUser::instance()->getListenPosVector() + css->Direction * (css->Dist + dist));
			vpos = _Position * (1-css->PosAlpha) + vpos*(css->PosAlpha);
			return vpos;
		}
	}

	return getPos();
}

void CStreamSource::play()
{
	nlassert(!_Playing);
	bool play = false;
	CAudioMixerUser *mixer = CAudioMixerUser::instance();
	
	{
		CAutoMutex<CMutex> autoMutex(m_BufferMutex);
		
		//if ((mixer->getListenPosVector() - _Position).sqrnorm() > m_StreamSound->getMaxDistance() * m_StreamSound->getMaxDistance())
		if ((_RelativeMode ? getPos().sqrnorm() : (mixer->getListenPosVector() - getPos()).sqrnorm()) > m_StreamSound->getMaxDistance() * m_StreamSound->getMaxDistance())
		{
			// Source is too far to play
			if (_Spawn)
			{
				if (_SpawnEndCb != NULL)
					_SpawnEndCb(this, _CbUserParam);
				delete this;
			}
			// nldebug("CStreamSource %p : play FAILED !", (CAudioMixerUser::IMixerEvent*)this);
			return;
		}
		
		CAudioMixerUser *mixer = CAudioMixerUser::instance();

		if (!hasPhysicalSource())
			initPhysicalSource();

		if (hasPhysicalSource())
		{
			ISource *pSource = getPhysicalSource();
			nlassert(pSource != NULL);
			
			for (uint i = 0; i < m_NextBuffer; ++i)
				pSource->submitStreamingBuffer(m_Buffers[i]);
			
			// pSource->setPos( _Position, false);
			pSource->setPos(getVirtualPos(), false);
			if (!m_Buffers[0]->isStereo())
			{
				pSource->setMinMaxDistances(m_StreamSound->getMinDistance(), m_StreamSound->getMaxDistance(), false);
				setDirection(_Direction); // because there is a workaround inside
				pSource->setVelocity(_Velocity);
			}
			pSource->setGain(_Gain);
			pSource->setSourceRelativeMode(_RelativeMode);
			// pSource->setLooping(_Looping);
			pSource->setPitch(_Pitch);
			pSource->setAlpha(m_Alpha);
			
			// and play the sound
			play = pSource->play();
			// nldebug("CStreamSource %p : REAL play done", (CAudioMixerUser::IMixerEvent*)this);
		}
		else
		{
			if (_Priority == HighestPri)
			{
				// This sound is not discardable, add it in waiting playlist
				mixer->addSourceWaitingForPlay(this);
				return;
			}
			else
			{
				// No source available, kill.
				if (_Spawn)
				{
					if (_SpawnEndCb != NULL)
						_SpawnEndCb(this, _CbUserParam);					
					delete this;
				}
				return;
			}
		}
		
		if (play)
			CSourceCommon::play();
	}

	nlassert(play);
}

/// Stop playing
void CStreamSource::stop()
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);
	
	// nldebug("CStreamSource %p : stop", (CAudioMixerUser::IMixerEvent*)this);
	// nlassert(_Playing);
	
	if (!_Playing)
		return;
	
	if (hasPhysicalSource())
		releasePhysicalSource();
	
	CSourceCommon::stop();

	m_FreeBuffers = 3;
	m_NextBuffer = 0;
	
	if (_Spawn)
	{
		if (_SpawnEndCb != NULL)
			_SpawnEndCb(this, _CbUserParam);
		delete this;
	}
}

void CStreamSource::setPos(const NLMISC::CVector& pos)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setPos(pos);
	if (hasPhysicalSource())
		getPhysicalSource()->setPos(getVirtualPos());
}

void CStreamSource::setVelocity(const NLMISC::CVector& vel)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setVelocity(vel);
	if (hasPhysicalSource())
		getPhysicalSource()->setVelocity(vel);
}

/*
 * Set the direction vector (3D mode only, ignored in stereo mode) (default: (0,0,0) as non-directional)
 */
void CStreamSource::setDirection(const NLMISC::CVector& dir)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setDirection(dir);

	// Set the direction
	if (hasPhysicalSource())
	{
		if (!m_Buffers[0]->isStereo())
		{
			static bool coneset = false;
			if (dir.isNull()) // workaround // For what?
			{
				getPhysicalSource()->setCone(float(Pi * 2), float(Pi * 2), 1.0f); // because the direction with 0 is not enough for a non-directional source!
				getPhysicalSource()->setDirection(CVector::I);  // Don't send a 0 vector, DSound will complain. Send (1,0,0), it's omnidirectional anyway.
				coneset = false;
			}
			else
			{
//				if (!coneset)
				{
					getPhysicalSource()->setCone(m_StreamSound->getConeInnerAngle(), m_StreamSound->getConeOuterAngle(), m_StreamSound->getConeOuterGain());
					coneset = true;
				}
				getPhysicalSource()->setDirection(dir);
			}
		}
	}
}

void CStreamSource::setGain(float gain)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setGain(gain);
	if (hasPhysicalSource())
		getPhysicalSource()->setGain(gain);
}

void CStreamSource::setRelativeGain(float gain)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setRelativeGain(gain);
	if (hasPhysicalSource())
		getPhysicalSource()->setGain(_Gain);
}

void CStreamSource::setPitch(float pitch)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setPitch(pitch);
	if (hasPhysicalSource())
		getPhysicalSource()->setPitch(pitch);
}

void CStreamSource::setSourceRelativeMode(bool mode)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);

	CSourceCommon::setSourceRelativeMode(mode);
	if (hasPhysicalSource())
		getPhysicalSource()->setSourceRelativeMode(mode);
}

/// Set the sample format. (channels = 1, 2, ...; bitsPerSample = 8, 16; frequency = samples per second, 44100, ...)
void CStreamSource::setFormat(uint8 channels, uint8 bitsPerSample, uint32 frequency)
{
	nlassert(!_Playing);

	m_Buffers[0]->setFormat(IBuffer::FormatPcm, channels, bitsPerSample, frequency);
	m_Buffers[1]->setFormat(IBuffer::FormatPcm, channels, bitsPerSample, frequency);
	m_Buffers[2]->setFormat(IBuffer::FormatPcm, channels, bitsPerSample, frequency);

	m_BytesPerSecond = ((uint)bitsPerSample * (uint)frequency * (uint)channels) / 8;
}

/// Return the sample format informations.
void CStreamSource::getFormat(uint8 &channels, uint8 &bitsPerSample, uint32 &frequency) const
{
	IBuffer::TBufferFormat bufferFormat;

	m_Buffers[0]->getFormat(bufferFormat, channels, bitsPerSample, frequency);
}

void CStreamSource::updateAvailableBuffers()
{
	if (hasPhysicalSource())
	{
		m_FreeBuffers = 3 - getPhysicalSource()->countStreamingBuffers();
	}
}

/// Get a writable pointer to the buffer of specified size. Use capacity to specify the required bytes. Returns NULL when all the buffer space is already filled. Call setFormat() first.
uint8 *CStreamSource::lock(uint capacity)
{
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);
	updateAvailableBuffers();
	if (m_FreeBuffers > 0)
		return m_Buffers[m_NextBuffer]->lock(capacity);
	return NULL;
}

/// Notify that you are done writing to the locked buffer, so it can be copied over to hardware if needed. Set size to the number of bytes actually written to the buffer. Returns true if ok.
bool CStreamSource::unlock(uint size)
{
	nlassert(m_FreeBuffers > 0);
	
	CAutoMutex<CMutex> autoMutex(m_BufferMutex);
	IBuffer *buffer = m_Buffers[m_NextBuffer];
	bool result = buffer->unlock(size);

	if (size > 0)
	{
		++m_NextBuffer; m_NextBuffer %= 3;
		--m_FreeBuffers;
		if (hasPhysicalSource())
			getPhysicalSource()->submitStreamingBuffer(buffer);
		m_LastSize = size;
	}

	return result;
}

/// Get the recommended buffer size to use with lock()/unlock()
void CStreamSource::getRecommendedBufferSize(uint &samples, uint &bytes) const
{
	IBuffer::TBufferFormat bufferFormat;
	uint8 channels;
	uint8 bitsPerSample;
	uint32 frequency;
	m_Buffers[0]->getFormat(bufferFormat, channels, bitsPerSample, frequency);

	samples = frequency / 25; // 25 is a good value
	bytes = ((uint)bitsPerSample * samples * (uint)channels) / 8;
}

/// Get the recommended sleep time based on the size of the last submitted buffer and the available buffer space
uint32 CStreamSource::getRecommendedSleepTime() const
{
	if (m_FreeBuffers > 0) return 0;
	uint32 sleepTime = (uint32)((1000.0f * ((float)m_LastSize) / (float)m_BytesPerSecond) / _Pitch);
	clamp(sleepTime, (uint32)0, (uint32)1000);
	return sleepTime;
}

/// Return if there are still buffers available for playback.
bool CStreamSource::hasFilledBuffersAvailable() const
{
	const_cast<CStreamSource *>(this)->updateAvailableBuffers();
	return m_FreeBuffers < 3;
}

} /* namespace NLSOUND */

/* end of file */
