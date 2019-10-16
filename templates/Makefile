#
# Extra lines needed for the makefile
#

#
# Replace the empty components line with ...
#
COMPONENTS=4343W CY8CPROTO_062_4343W FREERTOS

#
# Replace the empty defines line with the following two lines
#

MBEDTLSFLAGS = MBEDTLS_CONFIG_FILE='"libs/mbedtls-lwip-hal/config.h"'

# Add additional defines to the build process (without a leading -D).
DEFINES=$(MBEDTLSFLAGS) CYBSP_WIFI_CAPABLE CY_RETARGET_IO_CONVERT_LF_TO_CRLF

#
# Add these lines just below the POSTBUILD= line
#

#
# This is setup to build with either the IAR compiler or the
# GCC compiler.  There is currently no support for the ARM
# compiler.
#
ifeq ($(TOOLCHAIN),IAR)
CY_IGNORE+=./libs/FreeRTOS-Kernel/FreeRTOS/Source/portable/GCC
else
CY_IGNORE+=./libs/FreeRTOS-Kernel/FreeRTOS/Source/portable/IAR
endif
