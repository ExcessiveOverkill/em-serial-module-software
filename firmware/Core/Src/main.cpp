#include "main.h"

device* Device = nullptr;
#include "interrupt_catch.h"

int main(void){

	device dev = device();
	Device = &dev;

	Device->init();
	
	Device->run();

	return 1;	// should never reach this
}