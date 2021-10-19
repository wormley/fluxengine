#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "protocol.h"


int mainResetNormal(int argc, const char* argv[])
{
	usbRemoteReset(F_RESET_NORMAL);
    return 0;
}
int mainResetBootloader(int argc, const char* argv[])
{
	usbRemoteReset(F_RESET_BOOTLOADER);
    return 0;
}
