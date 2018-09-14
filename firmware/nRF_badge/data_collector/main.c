
#include "sdk_config.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#include "nrf_drv_common.h"

#include <stdarg.h>
//#include <nrf51822_peripherals.h> // needed for the peripheral defines!!! (e.g. UART_PRESENT) --> now in sdk_config.h
//#include <nrf51.h> // includes the core_cm0 peripheral (NVIC_SystemReset)
//#include <app_timer.h>
//#include <app_scheduler.h>

//#include <app_uart.h> // requires app_util_platform.h


#include "boards.h"

#include "nrf_drv_uart.h"
#include "nrf_gpio.h"

#include "nrf_delay.h"

#include "adc_lib.h"

#include "spi_lib.h"

//#include "uart_lib.h"

#include "softdevice_handler.h"
#include "flash_lib.h"

#include "eeprom_lib.h"

#include "debug_lib.h"

#include "storage1_lib.h"

#include "accel_lib.h"

#include "app_timer.h"
#include "systick_lib.h"

#include "ble_lib.h"

#include "app_error_weak.h"
#include "app_error.h"

/**
TODO:
	implement app_timer.h again self!!! with input paramter of the IRQ-PRIORITY!!!!
	it's ok because it is in the include path first!!! --> Not neeeded!!!

*/


#include "app_util_platform.h"

/**
 * From Nordic SDK
 */
//#include "nordic_common.h"
//#include "nrf.h"
//#include "nrf51_bitfields.h"

//#include "nrf_drv_rtc.h"        //driver abstraction for real-time counter
//#include "app_error.h"          //error handling
//#include "nrf_delay.h"          //includes blocking delay functions
//#include "nrf_gpio.h"           //abstraction for dealing with gpio
//#include "ble_flash.h"          //for writing to flash

//#include "app_error.h"

//#include "ble_gap.h"            //basic ble functions (advertising, scans, connecting)

//#include "debug_log.h"          //UART debugging logger
//requires app_fifo, app_uart_fifo.c and retarget.c for printf to work properly

//#include "nrf_drv_config.h"
//#include "boards.h"

/**
 * Custom libraries/abstractions
 */
//#include "analog.h"     //analog inputs, battery reading
//#include "battery.h"
//#include "external_flash.h"  //for interfacing to external SPI flash
//#include "scanner.h"       //for performing scans and storing scan data
//#include "self_test.h"   // for built-in tests
/*
typedef struct {
    bool error_occured;
    uint32_t error_code;
    uint32_t line_num;
    char file_name[32];
} AppErrorData_t;

static AppErrorData_t mAppErrorData __attribute((section (".noinit")));

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    mAppErrorData.error_occured = true;
    mAppErrorData.error_code = error_code;
    mAppErrorData.line_num = line_num;
    strncpy(mAppErrorData.file_name, (char *) p_file_name, sizeof(mAppErrorData.file_name));

    NVIC_SystemReset();
}
*/
//=========================== Global function definitions ==================================
//==========================================================================================

#define SCHED_MAX_EVENT_DATA_SIZE sizeof(uint32_t)
#define SCHED_QUEUE_SIZE 100



#define INIT_STRUCT(P_UART_BUFFER, RX_BUF_SIZE, TX_BUF_SIZE) \
    do                                                                                             \
    {                                                                                              \
        static uint8_t     rx_buf[RX_BUF_SIZE];                                                    \
        static uint8_t     tx_buf[TX_BUF_SIZE];                                                    \
                                                                                                   \
		(P_UART_BUFFER)->rx_buf      = rx_buf;                                                       \
        (P_UART_BUFFER)->rx_buf_size = sizeof (rx_buf);                                              \
        (P_UART_BUFFER)->tx_buf      = tx_buf;                                                       \
        (P_UART_BUFFER)->tx_buf_size = sizeof (tx_buf);                                              \
    } while (0)
		


typedef struct {
	uint8_t a;
	uint8_t b;
} test_struct;


nrf_drv_uart_t _instance;



uint8_t buf[200];
// https://stackoverflow.com/questions/4867229/code-for-printf-function-in-c
// https://devzone.nordicsemi.com/f/nordic-q-a/28999/using-floats-with-sprintf-gcc-arm-none-eabi-nrf51822
// If you want to have float support, add "LDFLAGS += -u _printf_float" in Makefile!



adc_instance_t mic_adc;
adc_instance_t bat_adc;

spi_instance_t acc_spi;

spi_instance_t ext_spi;



	//==== Funcion read registers ====
uint8_t readRegister8(uint8_t reg){
  uint8_t txBuf[1] = {reg | 0x80}; //Array to send
  uint8_t rxBuf[2] = {0,0}; //Array to receive
  spi_transmit_receive_bkgnd(&acc_spi, NULL, txBuf,sizeof(txBuf),rxBuf,2);
  
  //spi_master_send_recv(SPI_MASTER_0,txBuf,sizeof(txBuf),rxBuf,2); //Send and receive over SPI protocol
  //while(spi_busy()); //Wait while spi is bussy
  return rxBuf[1]; // Value retorned from spi register
}


/**
 * ============================================== MAIN ====================================================
 */
int main(void)
{

	
// TODO: - Move UART, SPI, ADC opertation enum from .h into .c and also the includes that needn't to be public! 

	nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn on LED
	
	nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_OFF);  //turn off LED

	ret_code_t ret;
	(void) ret;
	
	debug_init();
	

	
	debug_log("Start...\n\r");

	
	//nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2, LED_ON);  //turn on LED
	
	// Always initialize everything!!
	nrf_clock_lf_cfg_t clock_lf_cfg =  {.source        = NRF_CLOCK_LF_SRC_XTAL,            
										.rc_ctiv       = 0,                                
										.rc_temp_ctiv  = 0,                                
										.xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM};
										

	SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);


	
	
	
	
	ret = ble_init();
	debug_log("Ret ble_init: %d\n\r", ret);
	
	uint8_t mac[6];
	ble_get_MAC_address(mac);
	debug_log("MAC address: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	
	uint8_t advdata[31];
	
	ret = ble_set_advertising_custom_advdata(0xFF00, advdata, 13);
	debug_log("Ret ble_set_advertising_custom_advdata: %d\n\r", ret);
	
	ret = ble_start_advertising();
	debug_log("Ret ble_start_advertising: %d\n\r", ret);
	
	
	
	
	ret = ble_start_scanning(300, 100, 15);
	debug_log("Ret ble_start_scanning: %d\n\r", ret);
	nrf_delay_ms(5000);
	
	
	
	while(1);
	
	APP_TIMER_INIT(0, 50, NULL);
	
	ret = systick_init(0);
	
	
	debug_log("Ret systick: %u\n\r", ret);	
	debug_log("Millis: %u\n\r",  (uint32_t) systick_get_millis());
	
	systick_delay_millis(3*1000);
	uint64_t ms = systick_get_millis();
	uint32_t sec; uint16_t mil;
	systick_get_timestamp(&sec, &mil);
	debug_log("Millis: %u\n\r", (uint32_t) ms);
	
	
	debug_log("Timestamp: %u, %u\n\r", sec, mil);
	
	
	flash_init();
	
	flash_selftest();
	
	
	ret = eeprom_init();
	
	eeprom_selftest();
	
	ret = accel_init();
	nrf_delay_ms(2000);
	if(ret == NRF_SUCCESS) {
		nrf_gpio_pin_write(LED_2,LED_OFF);  //turn on LED
		nrf_delay_ms(500);
		nrf_gpio_pin_write(LED_2,LED_ON);  //turn on LED
		nrf_delay_ms(500);
		nrf_gpio_pin_write(LED_2,LED_OFF);  //turn on LED
	} else {		
		nrf_gpio_pin_write(LED_1,LED_OFF);  //turn on LED
		nrf_delay_ms(500);
		nrf_gpio_pin_write(LED_1,LED_ON);  //turn on LED
	}
	
	accel_selftest();
	
	while(1);
	
	

	
	//char data[] = "aaaaa\n\r";
	//char data_rx[1000];
	
		
	
	
	debug_log("Printf-Test: ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ%d\n\r", 11);
	
	
	
	debug_log("Ret init: %d\n\r", ret);
	
	/*
	while(1) {
		
		
		ret = uart_receive(&uart_instance, (uint8_t*) data_rx, 10);
		ret = uart_receive_buffer_bkgnd(&uart_instance, uart_handler);
		uart_transmit_done=0;
		//uart_transmit_bkgnd(&uart_instance, NULL, (uint8_t*) data_tx, 400);	
		uart_printf_bkgnd(&uart_instance, NULL, "Printf-Test: %d\n\r", ret);
		nrf_delay_ms(2);
		uart_printf_abort_bkgnd(&uart_instance);
		
		nrf_delay_ms(5000);
		//uart_printf(&uart_instance, "\n\rTransmit status (done: %d, %p), rxtx Pointer to data: %p, bytes transfered: %u\n\r", uart_transmit_done, &data_tx[0], uart_rxtx.p_data, uart_rxtx.bytes);
		//uart_printf(&uart_instance, "Transmit error  Mask: %u\n\r", uart_error.error_mask);
		
		
		//nrf_delay_ms(1000);
		
	}
	*/
	
	
	
	//nrf_drv_uart_t instance = NRF_DRV_UART_INSTANCE(1);	
	/*
	_instance.drv_inst_idx = UART0_INSTANCE_INDEX;//CONCAT_3(UART, 0, _INSTANCE_INDEX);
	_instance.reg.p_uart = (NRF_UART_Type *) NRF_UART0;//_BASE;
	nrf_drv_uart_init(&_instance, &config, handler);
	
	nrf_drv_uart_rx_enable(&_instance);
	nrf_drv_uart_rx(&_instance, rx_buffer, 1);
	
	uart_buffer_t uart_buffer;
	INIT_STRUCT(&uart_buffer, 10, 20);
	
	*/
	
	
	
	
	mic_adc.adc_peripheral = 0;
	mic_adc.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_8BIT;
	mic_adc.nrf_adc_config.scaling		= NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE;
	mic_adc.nrf_adc_config.reference	= MIC_AREF; // NRF_ADC_CONFIG_REF_EXT_REF1;
	mic_adc.nrf_adc_config_input		= MIC_PIN;	//NRF_ADC_CONFIG_INPUT_6; //ADC_CONFIG_PSEL_AnalogInput6;
	
	
	bat_adc.adc_peripheral = 0;
	bat_adc.nrf_adc_config.resolution 	= NRF_ADC_CONFIG_RES_10BIT;
	bat_adc.nrf_adc_config.scaling		= NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD;
	bat_adc.nrf_adc_config.reference	= NRF_ADC_CONFIG_REF_VBG; // NRF_ADC_CONFIG_REF_EXT_REF1;
	bat_adc.nrf_adc_config_input		= NRF_ADC_CONFIG_INPUT_DISABLED;	//ADC_CONFIG_PSEL_AnalogInput6;
	
	
	debug_log("Starting...\n");
	nrf_delay_ms(1000);
	
	adc_init(&mic_adc);
	
	adc_init(&bat_adc);
	int32_t val = 0;
	
	
	
	acc_spi.spi_peripheral = 0;
	acc_spi.nrf_drv_spi_config.frequency 	= NRF_DRV_SPI_FREQ_8M;
	acc_spi.nrf_drv_spi_config.bit_order 	= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
	acc_spi.nrf_drv_spi_config.mode			= NRF_DRV_SPI_MODE_3;
	acc_spi.nrf_drv_spi_config.orc			= 0;
	acc_spi.nrf_drv_spi_config.irq_priority = 3;
	acc_spi.nrf_drv_spi_config.ss_pin 		= 2;
	acc_spi.nrf_drv_spi_config.miso_pin		= 1;
	acc_spi.nrf_drv_spi_config.mosi_pin		= 4;
	acc_spi.nrf_drv_spi_config.sck_pin		= 3;
	
	ret = spi_init(&acc_spi);
	
	debug_log("SPI Init ret: %d\n\r", ret);
	
	ext_spi.spi_peripheral = 0;
	ext_spi.nrf_drv_spi_config.frequency 	= NRF_DRV_SPI_FREQ_8M;
	ext_spi.nrf_drv_spi_config.bit_order 	= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
	ext_spi.nrf_drv_spi_config.mode			= NRF_DRV_SPI_MODE_3;
	ext_spi.nrf_drv_spi_config.orc			= 0;
	ext_spi.nrf_drv_spi_config.irq_priority = 3;
	ext_spi.nrf_drv_spi_config.ss_pin 		= 0;
	ext_spi.nrf_drv_spi_config.miso_pin		= 1;
	ext_spi.nrf_drv_spi_config.mosi_pin		= 4;
	ext_spi.nrf_drv_spi_config.sck_pin		= 3;
	
	
	ret = spi_init(&ext_spi);
	
	debug_log("SPI Init ret: %d\n\r", ret);

	
	
	uint8_t read_byte = readRegister8(0x0F);
	(void) read_byte;
	
	debug_log("Read Byte: 0x%X\n\r", read_byte);
	
	while(1) {
		adc_read_raw(&mic_adc, &val);
		debug_log("Value: %d\n\r", val);
		nrf_delay_ms(2000);
		
		adc_read_raw(&bat_adc, &val);
		debug_log("Value: %d\n\r", val);
		nrf_delay_ms(2000);
		
		float voltage;
		adc_read_voltage(&bat_adc, &voltage, 1.2);
		debug_log("Voltage: %.3f\n\r", voltage);
		nrf_delay_ms(2000);
		
		
	}
	
	
	
	
	
	
	//app_error_save_and_stop(1,2,3);

	while(1);
	
/*
    #if defined(BOARD_PCA10028)  //NRF51DK
        //If button 4 is pressed on startup, do nothing (mostly so that UART lines are freed on the DK board)
        nrf_gpio_cfg_input(BUTTON_4,NRF_GPIO_PIN_PULLUP);  //button 4
        if(nrf_gpio_pin_read(BUTTON_4) == 0)  //button pressed
        {
            nrf_gpio_pin_dir_set(LED_4,NRF_GPIO_PIN_DIR_OUTPUT);
            nrf_gpio_pin_write(LED_4,LED_ON);
            while(1);
        }
        nrf_gpio_cfg_default(BUTTON_4);
    #endif

    debug_log_init();
    debug_log("\r\n\r\n\r\n\r\nUART trace initialized.\r\n\r\n");

    // TODO: Check the reset reason to make sure our noinit RAM is valid
    if (mAppErrorData.error_occured) {
        debug_log("CRASH! APP ERROR %lu @ %s:%lu\r\n", mAppErrorData.error_code,
                  mAppErrorData.file_name, mAppErrorData.line_num);
        mAppErrorData.error_occured = false;
    }

    debug_log("Name: %.5s\r\n",DEVICE_NAME);
    debug_log("Firmware Version: %s, Branch: %s, Commit: %s\r\n", GIT_TAG, GIT_BRANCH, GIT_COMMIT);


    // Define and set LEDs
    nrf_gpio_pin_dir_set(LED_1,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_1,LED_ON);  //turn on LED
    nrf_gpio_pin_dir_set(LED_2,NRF_GPIO_PIN_DIR_OUTPUT);  //set LED pin to output
    nrf_gpio_pin_write(LED_2,LED_OFF);  //turn off LED

    // Initialize
    BLE_init();
    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);  //set low power sleep mode
    adc_config();
    rtc_config();
    spi_init();


    #if defined(TESTER_ENABLE) // tester mode is enabled
        runSelfTests();
        while(1);
    #endif    // end of self tests




    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

    collector_init();
    storer_init();
    sender_init();
    scanner_init();
    BatteryMonitor_init();

    BLEsetBadgeAssignment(getStoredBadgeAssignment());
    advertising_init();

    // Blink once on start

	nrf_gpio_pin_write(LED_1,LED_OFF);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1, LED_ON);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LED_1, LED_OFF);
    nrf_delay_ms(200);

    nrf_gpio_pin_write(LED_1,LED_ON);
    nrf_gpio_pin_write(LED_2,LED_ON);
    nrf_delay_ms(100);
    nrf_gpio_pin_write(LED_1,LED_OFF);
    nrf_gpio_pin_write(LED_2,LED_OFF);


    nrf_delay_ms(1000);

    debug_log("Done with setup.  Entering main loop.\r\n\r\n");

    BLEstartAdvertising();

    nrf_delay_ms(2);

    while (true) {
        app_sched_execute();
        sd_app_evt_wait();
    }
	*/
}

/*
void BLEonConnect()
{
    debug_log("--CONNECTED--\r\n");
    sleep = false;

    // for app development. disable if forgotten in prod. version
    #ifdef DEBUG_LOG_ENABLE
        nrf_gpio_pin_write(LED_1,LED_ON);
    #endif

    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}

void BLEonDisconnect()
{
    debug_log("--DISCONNECTED--\r\n");
    sleep = false;

    // for app development. disable if forgotten in prod. version
    #ifdef DEBUG_LOG_ENABLE
        nrf_gpio_pin_write(LED_1,LED_OFF);
    #endif

    ble_timeout_cancel();
}

static void processPendingCommand(void * p_event_data, uint16_t event_size) {
    bool sendOperationsRemaining = updateSender();
    if (sendOperationsRemaining) {
        app_sched_event_put(NULL, 0, processPendingCommand);
    }
}


void BLEonReceive(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
    if(length > 0)
    {
        pendingCommand = unpackCommand(p_data, length);
        app_sched_event_put(NULL, 0, processPendingCommand);
    }

    ble_timeout_set(CONNECTION_TIMEOUT_MS);
}

*/

