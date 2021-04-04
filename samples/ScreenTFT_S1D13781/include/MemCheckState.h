#pragma once

#include <S1D13781/Driver.h>
#include <S1D13781/registers.h>
#include <HSPI/Test/MemCheckState.h>

class S1DMemCheckState : public HSPI::Test::MemCheckState
{
public:
	using MemCheckState::MemCheckState;

protected:
	void fillBlock(uint32_t addr, uint32_t* buffer, size_t size) override
	{
		if(addr < S1D13781_LUT1_BASE) {
			return MemCheckState::fillBlock(addr, buffer, size);
		}

		// LUT MSB isn't used, always reads 0
		uint32_t mask{0x00FFFFFF};
		for(unsigned i = 0; i < size; ++i) {
			buffer[i] = os_random() & mask;
		}
	}
};
