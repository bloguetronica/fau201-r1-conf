/* FAU201 Rev. 1 Configuration Program - Version 1.0 for Debian Linux
   Copyright (c) 2019 Samuel Lourenço

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


// Includes
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libusb-1.0/libusb.h>
#include "libusb-extra.h"

// Defines
#define EXIT_USERERR 2  // Exit status value to indicate a command usage error
#define TR_TIMEOUT 100  // Transfer timeout in milliseconds

// Function prototypes
bool is_otp_blank(libusb_device_handle *devhandle);
void lock_otp(libusb_device_handle *devhandle);
void reset(libusb_device_handle *devhandle);
void write_mfg_str1(libusb_device_handle *devhandle);
void write_mfg_str2(libusb_device_handle *devhandle);
void write_pin_cfg(libusb_device_handle *devhandle);
void write_prd_str1(libusb_device_handle *devhandle);
void write_prd_str2(libusb_device_handle *devhandle);
void write_ser_str(libusb_device_handle *devhandle, char *serial);
void write_usb_cfg(libusb_device_handle *devhandle);

// Global variables
int err_level = EXIT_SUCCESS;  // This variable is manipulated by other functions besides main()!

int main(int argc, char **argv)
{
    if (argc < 2)  // If the program was called without arguments
    {
        fprintf(stderr, "Error: Missing argument.\nUsage: fau201-r0-conf SERIALNUMBER\n");
        err_level = EXIT_USERERR;
    }
    else
    {
        libusb_context *context;
        if (libusb_init(&context) != 0)  // Initialize libusb. In case of failure
        {
            fprintf(stderr, "Error: Could not initialize libusb.\n");
            err_level = EXIT_FAILURE;
        }
        else  // If libusb is initialized
        {
            libusb_device_handle *devhandle = libusb_open_device_with_vid_pid_serial(context, 0x10C4, 0x87A0, (unsigned char *)argv[1]);  // Open the device having the specified serial number, and get the device handle
            if (devhandle == NULL)  // If the previous operation fails to get a device handle
            {
                fprintf(stderr, "Error: Could not find device.\n");
                err_level = EXIT_FAILURE;
            }
            else  // If the device is successfully opened and a handle obtained
            {
                bool kernel_attached = false;
                if (libusb_kernel_driver_active(devhandle, 0) != 0)  // If a kernel driver is active on the interface
                {
                    libusb_detach_kernel_driver(devhandle, 0);  // Detach the kernel driver
                    kernel_attached = true;  // Flag that the kernel driver was attached
                }
                if (libusb_claim_interface(devhandle, 0) != 0)  // Claim the interface. In case of failure
                {
                    fprintf(stderr, "Error: Device is currently unavailable.\n");
                    err_level = EXIT_FAILURE;
                }
                else  // If the interface is successfully claimed
                {
                    if (is_otp_blank(devhandle) && err_level == 0)  // Check if the OTP ROM is blank (err_level can change to 1 as a consequence of that verification, hence the need for "&& err_level == 0" in order to avoid misleading messages)
                    {
                        printf("Device is blank.\nDo you wish to configure it? [y/N] ");
                        char cin = getc(stdin);  // Get character entered by user
                        if (cin == 'Y' || cin == 'y')  // If user entered "Y" or "y"
                        {
                            char serial[6];
                            srand(time(NULL));  // Generate a random seed
                            for (short i = 0; i < 6; ++i)  // Generate serial number (last 6 characters)
                            {
                                short random = rand() % 36;
                                serial[i] = (char)(random < 10 ? random + 48 : random + 55);
                            }
                            write_usb_cfg(devhandle);  // Write USB configuration values
                            write_mfg_str1(devhandle);  // Write manufacturing string
                            write_mfg_str2(devhandle);
                            write_prd_str1(devhandle);  // Write product string
                            write_prd_str2(devhandle);
                            write_ser_str(devhandle, serial);  // Write serial number
                            write_pin_cfg(devhandle);  // Write pin configuration values
                            if (err_level == 0)  // If all goes well
                                lock_otp(devhandle);  // Lock the OTP ROM
                            reset(devhandle);  // Reset the device
                            if (err_level == 0)  // If all still goes well
                                printf("Device is now configured.\n");  // Notice that no verification is done after reset, since the device has to be allowed to re-enumerate before getting the updated register values
                        }   
                        else  // If user entered any other character
                            printf("Device configuration canceled.\n");
                    }
                    else if (err_level == 0)  // If all goes well
                        printf("Device is not blank.\n");
                    libusb_release_interface(devhandle, 0);  // Release the interface
                }
                if (kernel_attached)  // If a kernel driver was attached to the interface before
                    libusb_attach_kernel_driver(devhandle, 0);  // Reattach the kernel driver
                libusb_close(devhandle);  // Close the device
            }
            libusb_exit(context);  // Deinitialize libusb
        }
    }
    return err_level;
}

bool is_otp_blank(libusb_device_handle *devhandle)  // Checks if the OTP ROM of the CP2130 is blank
{
    unsigned char control_buf_in[2];
    if (libusb_control_transfer(devhandle, 0xC0, 0x6E, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in))
    {
        fprintf(stderr, "Error: Failed control transfer (0xC0, 0x6E).\n");
        err_level = EXIT_FAILURE;
    }
    return (control_buf_in[0] == 0xFF && control_buf_in[1] == 0xFF);  // Returns one if both lock bytes are set to 0xFF, that is, the OPT ROM is blank
}

void lock_otp(libusb_device_handle *devhandle)  // Locks the OTP ROM on the CP2130
{
    unsigned char control_buf_out[2] = {
        0x00, 0x00  // Values to be written into the lock bytes, so that both are set to zero
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x6F, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x6F).\n");
        err_level = EXIT_FAILURE;
    }
}

void reset(libusb_device_handle *devhandle)  // Issues a reset to the CP2130, which in effect resets the entire device
{
    if (libusb_control_transfer(devhandle, 0x40, 0x10, 0x0000, 0x0000, NULL, 0, TR_TIMEOUT) != 0)
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x10).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_mfg_str1(libusb_device_handle *devhandle)  // Writes the first half of the manufacturing string into the OTP ROM
{
    unsigned char control_buf_out[64] = {
        0x1E, 0x03, 0x42, 0x00, 0x6C, 0x00, 0x6F, 0x00,  // Bloguetrónica
        0x67, 0x00, 0x75, 0x00, 0x65, 0x00, 0x74, 0x00,
        0x72, 0x00, 0xF3, 0x00, 0x6E, 0x00, 0x69, 0x00,
        0x63, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x63, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x63).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_mfg_str2(libusb_device_handle *devhandle)  // Writes the second half of the manufacturing string into the OTP ROM
{
    unsigned char control_buf_out[64] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x65, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x65).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_pin_cfg(libusb_device_handle *devhandle)  // Writes the pin configuration values into the OTP ROM
{
    unsigned char control_buf_out[20] = {
        0x03,        // GPIO.O as !CS0
        0x00,        // GPIO.1 as input (not used)
        0x00,        // GPIO.2 as input (not used)
        0x00,        // GPIO.3 as input (not used)
        0x00,        // GPIO.4 as input (not used)
        0x00,        // GPIO.5 as input (not used)
        0x00,        // GPIO.6 as input (not used)
        0x00,        // GPIO.7 as input (not used)
        0x04,        // GPIO.8 as SPIACT
        0x04,        // GPIO.9 as SUSPEND
        0x04,        // GPIO.10 as !SUSPEND
        0x00, 0x00,  // Suspend pin level
        0x00, 0x00,  // Suspend pin mode
        0x00, 0x00,  // Wakeup pin mask
        0x00, 0x00,  // Wakeup pin match
        0x00         // Clock divider set to 256
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x6D, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x6D).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_prd_str1(libusb_device_handle *devhandle)  // Writes the first half of the product string into the OTP ROM
{
    unsigned char control_buf_out[64] = {
        0x2A, 0x03, 0x46, 0x00, 0x41, 0x00, 0x55, 0x00,  // FAU201 Power Supply
        0x32, 0x00, 0x30, 0x00, 0x31, 0x00, 0x20, 0x00,
        0x50, 0x00, 0x6F, 0x00, 0x77, 0x00, 0x65, 0x00,
        0x72, 0x00, 0x20, 0x00, 0x53, 0x00, 0x75, 0x00,
        0x70, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x79, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x67, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x67).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_prd_str2(libusb_device_handle *devhandle)  // Writes the second half of the product string into the OTP ROM
{
    unsigned char control_buf_out[64] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x69, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x69).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_ser_str(libusb_device_handle *devhandle, char *serial)  // Writes the serial string into the OTP ROM
{
    unsigned char control_buf_out[64] = {
        0x1C, 0x03, 0x46, 0x00, 0x32, 0x00, 0x31, 0x00,  // F21-01xxxxxx
        0x2D, 0x00, 0x30, 0x00, 0x31, 0x00,
        serial[0], 0x00,
        serial[1], 0x00,
        serial[2], 0x00,
        serial[3], 0x00,
        serial[4], 0x00,
        serial[5], 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x6B, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x6B).\n");
        err_level = EXIT_FAILURE;
    }
}

void write_usb_cfg(libusb_device_handle *devhandle)  // Writes the USB configuration values into the OTP ROM
{
    unsigned char control_buf_out[10] = {
        0xC4, 0x10,  // VID is 0x10C4
        0x46, 0x8C,  // PID is 0x8C46
        0xFA,        // Maximum power is 500mA (0xFA)
        0x00,        // USB bus powered with voltage regulator enabled
        0x01,        // Major release number
        0x01,        // Minor release number
        0x01,        // High priority write
        0x9F         // Write relevant fields
    };
    if (libusb_control_transfer(devhandle, 0x40, 0x61, 0xA5F1, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out))
    {
        fprintf(stderr, "Error: Failed control transfer (0x40, 0x61).\n");
        err_level = EXIT_FAILURE;
    }
}
