#include <SmingCore.h>
#include <Platform/Timers.h>
#include <demo.h>

HSPI::Controller spi;
S1D13781::Gfx graphics(spi);
DisplayDemo demo(graphics);

// Chip selects to use for our devices
#define CS_LCD 2

#ifdef ARCH_ESP32
#define SPI_PINSET HSPI::PinSet::normal
#else
#define SPI_PINSET HSPI::PinSet::overlap
#endif

void init()
{
// Start serial for serial monitor
// CS #1 conflicts with regular serial TX pin, so switch to secondary debug
#if !defined(ENABLE_GDB) && !defined(ARCH_HOST) && (CS_LCD == 1)
	Serial.setPort(UART_ID_1);
#endif
	Serial.setTxBufferSize(2048);
	Serial.setTxWait(true);
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true);

	spi.begin();

	// Initialise display and start demo
	if(!graphics.begin(SPI_PINSET, CS_LCD, 27000000U)) {
		debug_e("Failed to start LCD device");
	} else {
		debug_i("LCD clock = %u", graphics.getSpeed());
		//		graphics.setRotation(S1D13781::Window::main, 0);
		demo.start();
	}
}
