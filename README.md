# freertos-lwip-mbedtls

This repository contains ModusToolbox 2.0 library files and template code that will allow you to add LWIP, MBED TLS, and the Cypress wireless host driver to a FreeRTOS projecct.

# Usage
To use this do the following three steps
1. create a file called "freertos-lwip-mbedtls.lib" inside of your project
2. Add the URL for this repo to the file "https://github.com/iotexpert/freertos-lwip-mbedtls/#master"
3. run "make getlibs"

# Functionality
This library file will bring in the following libraries
* lwip - Master branch from the LWIP github respository
* lwip configuration - Configuration files required for LWIP
* mbedtls - Master branch from the ARM github respository
* mbedtls-lwip-hal - configuration code to connect lwip, mbedtls and the Cypress hal
* Cypress Wireless Host Driver - from the Cypress GitHub respository
* Cypress RTOS Abstraction Layer - from the Cypress GitHub respository
* FreeRTOS - From the Cypress GitHub Respository, same as the library manager brings in
* FreeRTOS-LWIP-WHD - ?

# Example Code
```
some code code
```

# Template Code

# Developement Kits
* CY8CPROTO_062_4343W
* CY8CKIT_062S2_43012
