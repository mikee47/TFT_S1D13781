/** @brief A monochrome bitmap, used for building font source data */
#pragma once

#include <stdint.h>
#include <stdlib.h>

class BitBuffer
{
public:
	~BitBuffer()
	{
		free(buffer);
	}

	bool initialise(unsigned w, unsigned h)
	{
		// Built text in buffer, 1 bit per pixel
		unsigned newSize = ((w * h) + 7) / 8;
		if(newSize > bufSize) {
			auto newBuffer = static_cast<uint8_t*>(realloc(buffer, newSize));
			if(newBuffer == nullptr) {
				return false;
			}
			buffer = newBuffer;
			bufSize = newSize;
		}
		memset(buffer, 0, newSize);
		bitPos = 0;
		return true;
	}

	void setPixel(bool state)
	{
		if(state) {
			buffer[bitPos / 8] |= 0x01 << (bitPos % 8);
		}
		++bitPos;
	}

	void skip(unsigned count)
	{
		bitPos += count;
	}

	void* getPtr()
	{
		return buffer;
	}

	unsigned getSize()
	{
		return bufSize;
	}

	unsigned getPos()
	{
		return (bitPos + 7) / 8;
	}

private:
	uint8_t* buffer = nullptr;
	uint16_t bufSize = 0;
	uint16_t bitPos = 0;
};

