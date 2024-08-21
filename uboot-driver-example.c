/*
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <microkit.h>
#include <sel4_dma.h>
#include <uboot_drivers.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio_microkit.h>
#include <sel4_timer.h>

#define CONFIG_PLAT_MAAXBOARD

/* Determine which functionality to test based upon the platform */
#if defined(CONFIG_PLAT_MAAXBOARD)
    #define TEST_CLK
    #define TEST_CLOCKS
    #define TEST_SPI
    #define TEST_LED
    #define TEST_LED_NAME_1 "usr_led"
    #define TEST_LED_NAME_2 "sys_led"
    #define TEST_I2C
    #define TEST_ETHERNET
    #define TEST_USB
    #define TEST_MMC
    #define TEST_PINMUX
    #define TEST_GPIO
    #define TEST_FILESYSTEM
    #define TEST_FILESYSTEM_PARTITION "mmc 0:1"  // Partition 1 on mmc device 0
    #define TEST_FILESYSTEM_FILENAME  "test_file.txt"

#elif defined(CONFIG_PLAT_ODROIDC2)
    #define TEST_PINMUX
    #define TEST_GPIO
    #define TEST_LED
    #define TEST_LED_NAME_1 "c2:blue:alive"
    #define TEST_LED_NAME_2 "c2:blue:alive"

#else
    /* Enable all tests for unrecognised platform */
    #define TEST_CLK
    #define TEST_CLOCKS
    #define TEST_SPI
    #define TEST_LED
    #define TEST_LED_NAME_1 "dummy_led"
    #define TEST_LED_NAME_2 "dummy_led"
    #define TEST_I2C
    #define TEST_ETHERNET
    #define TEST_USB
    #define TEST_MMC
    #define TEST_PINMUX
    #define TEST_GPIO
    #define TEST_FILESYSTEM
    #define TEST_FILESYSTEM_PARTITION "dummy_partition"
    #define TEST_FILESYSTEM_FILENAME  "test_file.txt"
#endif

// fdt initialise 
#define STR2(x) #x
#define STR(x) STR2(x)
#define INCBIN_SECTION ".rodata"
#define INCBIN(name, file) \
    __asm__(".section " INCBIN_SECTION "\n" \
            ".global incbin_" STR(name) "_start\n" \
            ".balign 16\n" \
            "incbin_" STR(name) "_start:\n" \
            ".incbin \"" file "\"\n" \
            \
            ".global incbin_" STR(name) "_end\n" \
            ".balign 1\n" \
            "incbin_" STR(name) "_end:\n" \
            ".byte 0\n" \
    ); \
    extern __attribute__((aligned(16))) const char incbin_ ## name ## _start[]; \
    extern                              const char incbin_ ## name ## _end[] 
INCBIN(device_tree, DTB_PATH); 

const char* _end = incbin_device_tree_end;

#define REG_TIMER_PATH      "/soc@0/bus@30400000/timer@306a0000"
#define REG_CCM_PATH        "/soc@0/bus@30000000/clock-controller@30380000"
#define REG_IOMUXC_PATH     "/soc@0/bus@30000000/iomuxc@30330000"
#define REG_OCOTP_PATH      "/soc@0/bus@30000000/ocotp-ctrl@30350000"
#define REG_SYSCON_PATH     "/soc@0/bus@30000000/syscon@30360000"
#define REG_USB_2_PATH      "/soc@0/usb@38200000"
#define REG_USB_PHY_2_PATH  "/soc@0/usb-phy@382f0040"
#define REG_MMC_PATH        "/soc@0/bus@30800000/mmc@30b40000"
#define REG_ETH_PATH        "/soc@0/bus@30800000/ethernet@30be0000"
#define REG_GPIO_1_PATH     "/soc@0/bus@30000000/gpio@30200000"
#define REG_GPIO_2_PATH     "/soc@0/bus@30000000/gpio@30210000"
#define REG_GPIO_3_PATH     "/soc@0/bus@30000000/gpio@30220000"
#define REG_GPIO_4_PATH     "/soc@0/bus@30000000/gpio@30230000"
#define REG_GPIO_5_PATH     "/soc@0/bus@30000000/gpio@30240000"
#define REG_I2C_0_PATH      "/soc@0/bus@30800000/i2c@30a20000"
#define REG_I2C_1_PATH      "/soc@0/bus@30800000/i2c@30a30000"
#define REG_I2C_2_PATH      "/soc@0/bus@30800000/i2c@30a40000"
#define REG_I2C_3_PATH      "/soc@0/bus@30800000/i2c@30a50000"
#define REG_SPI_0_PATH      "/soc@0/bus@30800000/spi@30820000"


/* List the set of device tree paths for the devices we wish to access.
 * Note these need ot be the root nodes of each device which the
 * the library supplies a driver */

#define DEV_TIMER_PATH      REG_TIMER_PATH
#define DEV_CCM_PATH        REG_CCM_PATH
#define DEV_IOMUXC_PATH     REG_IOMUXC_PATH
#define DEV_OCOTP_PATH      REG_OCOTP_PATH
#define DEV_SYSCON_PATH     REG_SYSCON_PATH
#define DEV_USB_2_PATH      REG_USB_2_PATH
#define DEV_USB_PHY_2_PATH  REG_USB_PHY_2_PATH
#define DEV_MMC_PATH        REG_MMC_PATH
#define DEV_ETH_PATH        REG_ETH_PATH
#define DEV_GPIO_1_PATH     REG_GPIO_1_PATH
#define DEV_GPIO_2_PATH     REG_GPIO_2_PATH
#define DEV_GPIO_3_PATH     REG_GPIO_3_PATH
#define DEV_GPIO_4_PATH     REG_GPIO_4_PATH
#define DEV_GPIO_5_PATH     REG_GPIO_5_PATH
#define DEV_I2C_0_PATH      REG_I2C_0_PATH
#define DEV_I2C_1_PATH      REG_I2C_1_PATH
#define DEV_I2C_2_PATH      REG_I2C_2_PATH
#define DEV_I2C_3_PATH      REG_I2C_3_PATH
#define DEV_SPI_0_PATH      REG_SPI_0_PATH
#define DEV_LEDS_PATH       "/leds"
#define DEV_CLK_1_PATH      "/clock-ckil"
#define DEV_CLK_2_PATH      "/clock-osc-25m"
#define DEV_CLK_3_PATH      "/clock-osc-27m"
#define DEV_CLK_4_PATH      "/clock-ext1"
#define DEV_CLK_5_PATH      "/clock-ext2"
#define DEV_CLK_6_PATH      "/clock-ext3"
#define DEV_CLK_7_PATH      "/clock-ext4"

#define DEV_PATH_COUNT 27

#define DEV_PATHS {                                                             \
    DEV_USB_2_PATH,                                                             \
    DEV_USB_PHY_2_PATH,                                                         \
    DEV_MMC_PATH,                                                               \
    DEV_ETH_PATH,                                                               \
    DEV_TIMER_PATH,                                                             \
    DEV_CCM_PATH,                                                               \
    DEV_OCOTP_PATH,                                                             \
    DEV_SYSCON_PATH,                                                            \
    DEV_IOMUXC_PATH,                                                            \
    DEV_GPIO_1_PATH,                                                            \
    DEV_GPIO_2_PATH,                                                            \
    DEV_GPIO_3_PATH,                                                            \
    DEV_GPIO_4_PATH,                                                            \
    DEV_GPIO_5_PATH,                                                            \
    DEV_I2C_0_PATH,                                                             \
    DEV_I2C_1_PATH,                                                             \
    DEV_I2C_2_PATH,                                                             \
    DEV_I2C_3_PATH,                                                             \
    DEV_SPI_0_PATH,                                                             \
    DEV_LEDS_PATH,                                                              \
    DEV_CLK_1_PATH,                                                             \
    DEV_CLK_2_PATH,                                                             \
    DEV_CLK_3_PATH,                                                             \
    DEV_CLK_4_PATH,                                                             \
    DEV_CLK_5_PATH,                                                             \
    DEV_CLK_6_PATH,                                                             \
    DEV_CLK_7_PATH                                                              \
    };

// picolibc setup
seL4_IPCBuffer* __sel4_ipc_buffer_obj;

static ps_dma_man_t dma_manager;

// DMA state
uintptr_t dma_base;
uintptr_t dma_cp_paddr;
size_t dma_size = 0x100000;

void
init(void)
{
    const char *const_dev_paths[] = DEV_PATHS;

    // Initalise DMA manager
    microkit_dma_manager(&dma_manager);

    // Initialise DMA
    microkit_dma_init(dma_base, dma_size,
        4096, 1);

    // Initialise uboot library
    initialise_uboot_drivers(
    dma_manager,
    incbin_device_tree_start,
    /* List the device tree paths for the devices */
    const_dev_paths, DEV_PATH_COUNT);

    run_uboot_command("dm tree");

    #ifdef TEST_CLK
    run_uboot_command("clk dump");
    #endif

    #ifdef TEST_SPI
    printf("Initialising BMP280 sensor on SPI bus (if connected):\n");
    // Read id register of device on SPI bus 0
    // For the BMP280 sensor, expect to return id of 0x58
    run_uboot_command("sspi 0:0.3@1000000 16 D000");

    // Initialise BMP280 prior to reading temperature register
    run_uboot_command("sspi 0:0.3@1000000 16 7508");
    run_uboot_command("sspi 0:0.3@1000000 16 7437");

    // The raw data does not correspond to meaningful temperatures without further processing
    printf("Raw temperature data from BMP280 sensor:\n");
    for (int x=0; x<=10; x++) {
        run_uboot_command("sspi 0:0.3@1000000 24 FA0000");
    }
    #endif

    #ifdef TEST_CLOCKS
    run_uboot_command("clocks");
    #endif

    #ifdef TEST_LED
    run_uboot_command("led list");

    // Flash the LEDs
    for (int x=0; x<4; x++) {
        wrap_mdelay(125);
        run_uboot_command("led "TEST_LED_NAME_1" off");
        wrap_mdelay(125);
        run_uboot_command("led "TEST_LED_NAME_2" on");
        wrap_mdelay(125);
        run_uboot_command("led "TEST_LED_NAME_1" on");
        wrap_mdelay(125);
        run_uboot_command("led "TEST_LED_NAME_2" off");
    }
    #endif

    #ifdef TEST_I2C
    // Probe and read device already present on MaaXBoard I2C bus
    run_uboot_command("i2c dev 0");
    // Probing I2C bus 0 should return chip address of 0x4b for BD71837MWV Power Management IC
    run_uboot_command("i2c probe");
    run_uboot_command("i2c md 0x4b 0x0.1 0x20");
    #endif

    #ifdef TEST_ETHERNET
    /* Edit the following for your IP addresses:
     * run_uboot_command("setenv ipaddr xxx.xxx.xxx.xxx"); // IP address to allocate to MaaXBoard
     * run_uboot_command("ping yyy.yyy.yyy.yyy"); // IP address of host machine
     */
    run_uboot_command("setenv ipaddr 192.168.1.106");
    run_uboot_command("ping 10.0.0.253");

    /* Optionally, to ping to an address beyond the local network:
     * run_uboot_command("setenv gatewayip zzz.zzz.zzz.zzz"); // IP address of router
     * run_uboot_command("setenv netmask 255.255.255.0");
     * run_uboot_command("ping 8.8.8.8"); // An example internet IP address (Google DNS)
     */
    #endif

    #ifdef TEST_USB
    // USB operations
    run_uboot_command("setenv stdin usbkbd"); // Use a USB keyboard as the input device

    run_uboot_command("usb start");

    run_uboot_command("part list usb 0");

    run_uboot_command("fatls usb 0");
    #endif

    #ifdef TEST_MMC
    // SD/MMC operations
    run_uboot_command("mmc info");

    run_uboot_command("part list mmc 0");

    run_uboot_command("fatls mmc 0");
    #endif
    
    #ifdef TEST_PINMUX
    run_uboot_command("pinmux status -a");
    #endif

    #ifdef TEST_GPIO
    run_uboot_command("gpio status -a");
    #endif

    run_uboot_command("dm tree");


    #ifdef TEST_FILESYSTEM
    /* Example of filesystem handling. Writes a file to a FAT partition before reading
     * the contents back and deleting the file. */

    // String to build the U-Boot command in
    char uboot_cmd[64];

    // Test string to write to the file
    const char test_string[] = "Hello file!";

    // Test string to read the file into
    #define MAX_BYTES_TO_READ 32
    char read_string[MAX_BYTES_TO_READ];

    // Build command to write the test string to a file and execute command
    sprintf(uboot_cmd, "fatwrite %s 0x%x %s %x %x",
        TEST_FILESYSTEM_PARTITION,   // The U-Boot partition designation
        &test_string,                // Address of the data to write
        TEST_FILESYSTEM_FILENAME,    // Filename to write to (or create)
        strlen(test_string),         // The number of bytes to write
        0);                          // The offset in the file to start writing from
    run_uboot_command(uboot_cmd);

    // Read then output contents of the file
    sprintf(uboot_cmd, "fatload %s 0x%x %s %x %x",
        TEST_FILESYSTEM_PARTITION,   // The U-Boot partition designation
        &read_string,                // Address to read the data into
        TEST_FILESYSTEM_FILENAME,    // Filename to read from
        MAX_BYTES_TO_READ,           // Max number of bytes to read (0 = to end of file)
        0);                          // The offset in the file to start read from
    run_uboot_command(uboot_cmd);
    printf("String read from file %s: %s\n", TEST_FILESYSTEM_FILENAME, read_string);

    // Now delete the temporary file used for the example
    sprintf(uboot_cmd, "fatrm %s %s", TEST_FILESYSTEM_PARTITION, TEST_FILESYSTEM_FILENAME);
    run_uboot_command(uboot_cmd);
    #endif

    #ifdef TEST_USB

    run_uboot_command("usb tree");

    run_uboot_command("usb info");

    // Loop for a while reading keypresses and echoing to screen
    printf("Echoing input from the USB keyboard:\n");
    for (int x=0; x<=1000; x++) {
        while (uboot_stdin_tstc() > 0)
            printf("Received character: %c\n", uboot_stdin_getc(), stdout);
        udelay(10000);
    }

    run_uboot_command("usb stop");
    #endif

    shutdown_uboot_drivers();

    printf("Completed U-Boot driver example\n");

    // Loop forever
    while (1);
}

void
notified(microkit_channel ch)
{
}