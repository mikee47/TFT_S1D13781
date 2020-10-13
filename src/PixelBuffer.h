/** @brief Class to manipulate a rectangular buffer of pixels */
#pragma once

#include <S1D13781/SeColor.h>
#include <debug_progmem.h>

class PixelBuffer
{
public:
	~PixelBuffer()
	{
		free(buffer);
	}

	bool initialise(unsigned w, unsigned h, ImageDataFormat format)
	{
		unsigned bpp = ::getBytesPerPixel(format);
		unsigned newStride = w * bpp;
		unsigned newSize = h * newStride;
		if(newSize > bufSize) {
			auto newBuffer = static_cast<uint8_t*>(realloc(buffer, newSize));
			if(newBuffer == nullptr) {
				return false;
			}
			buffer = newBuffer;
			bufSize = newSize;
		}
		width = w;
		height = h;
		this->format = format;
		bytesPerPixel = bpp;
		stride = newStride;
		return true;
	}

	uint8_t getBytesPerPixel()
	{
		return bytesPerPixel;
	}

	void* getPtr()
	{
		return buffer;
	}

	unsigned getSize()
	{
		return height * stride;
	}

	unsigned getStride()
	{
		return stride;
	}

	unsigned getOffset(unsigned x, unsigned y)
	{
		return (y * stride) + (x * bytesPerPixel);
	}

	SeColor getPixel(unsigned offset)
	{
		checkOffset(offset);
		SeColor color(0, format);
		if(bytesPerPixel >= 3) {
			color.code |= buffer[offset + 2] << 16;
		}
		if(bytesPerPixel >= 2) {
			color.code |= buffer[offset + 1] << 8;
		}
		color.code |= buffer[offset];
		return color;
	}

	SeColor getPixel(unsigned x, unsigned y)
	{
		return getPixel(getOffset(x, y));
	}

	void setPixel(unsigned offset, SeColor color)
	{
		checkOffset(offset);
		if(bytesPerPixel >= 3) {
			buffer[offset + 2] = (color.code >> 16) & 0xff;
		}
		if(bytesPerPixel >= 2) {
			buffer[offset + 1] = (color.code >> 8) & 0xff;
		}
		buffer[offset] = color.code & 0xff;
	}

	void setPixel(unsigned x, unsigned y, SeColor color)
	{
		setPixel(getOffset(x, y), color);
	}

	void fill(SeColor color)
	{
		unsigned size = getSize();
		for(unsigned offset = 0; offset < size; offset += bytesPerPixel) {
			setPixel(offset, color);
		}
	}

	void print(const char* tag, unsigned addr)
	{
		//		m_printHex("buf", buffer, getSize());

		unsigned bufSize = width + 30;
		auto buf = new char[bufSize];
		for(unsigned y = 0; y < height; ++y) {
			char* p = buf;
			p += m_snprintf(p, bufSize, "%s %06x ", tag, addr);

			for(unsigned x = 0; x < width; ++x) {
				SeColor color = getPixel(x, y);
				uint8_t r = (color.code >> 5) & 0x07;
				uint8_t g = (color.code >> 2) & 0x07;
				uint8_t b = (color.code & 0x03) << 1;
				//		uint8_t val = (cl == 0) ? r : (cl == 1) ? g : b;
				unsigned val = std::max(std::max(r, g), b);
				*p++ = val ? ('0' + val) : ' ';
				//		*p++ = (grey > 3) ? 'X' : ' ';
			}
			*p++ = '\n';
			m_nputs(buf, p - buf);

			addr += stride;
		}

		delete[] buf;
		//m_printHex(s, destBuffer.getPtr(), destBuffer.getStride());
	}

	void checkOffset(unsigned offset)
	{
		if(offset + bytesPerPixel > getSize()) {
			debug_e("BAD OFFSET %u", offset);
		}
	}

private:
	uint8_t* buffer = nullptr;
	uint16_t bufSize = 0;
	uint16_t width = 0;
	uint16_t height = 1;
	uint16_t stride = 0;
	uint8_t bytesPerPixel = 0;
	ImageDataFormat format = format_RGB_888;
};
