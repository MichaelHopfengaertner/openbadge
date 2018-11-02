#ifndef __FLASH_LIB_H
#define __FLASH_LIB_H


// https://devzone.nordicsemi.com/f/nordic-q-a/18083/fds---pstorage---fstorage---which-to-use
// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v12.0.0%2Flib_fstorage.html&cp=4_0_0_3_32


/** @file
 *
 * @brief Internal flash abstraction library.
 *
 * @details It enables to erase pages, store and read word data into the flash memory by using the fstorage-library.
 *			The erase and store operations are asynchronous/non-blocking functions. To check the status of the operation,
 *			just call flash_get_operation().
 *			This underlying fstorage module uses the softdevice, so the application has to initialize it before using this module. 
 *			Furthermore, for retrieving system events (needed by fstorage) the system_event_lib-module is used.
 *
 * @note    It is important to erase a flash page before storing data into it. This library doesn't report an error, if the data haven't been stored correctly.
 *			In case the flash page was erased before, the data should actually be stored correctly, but to be on the safe side, read out the data again and check.
 *
 */


#include <stdbool.h>
//#include "sdk_common.h"	// Needed for the definition of ret_code_t and the error-codes
#include "sdk_errors.h"		// Needed for the definition of ret_code_t and the error-codes


// TODO: define this by the linker script with enough space for new program code!
#ifdef UNIT_TEST
#define FLASH_NUM_PAGES 30	
#else
#ifdef DEBUG_LOG_ENABLE
#define FLASH_NUM_PAGES 70
#else
#define FLASH_NUM_PAGES 70
#endif
#endif
	
#if   	defined (NRF51)
#define FLASH_PAGE_SIZE_WORDS	256						/**< Number of words in one page (nrf51: 256) */
#elif (defined (NRF52) || defined(NRF52840_XXAA))
#define FLASH_PAGE_SIZE_WORDS	1024					/**< Number of words in one page (nrf52: 1024) */
#endif


/**@brief The different EEPROM operations. These operations will be used to set the peripheral busy or not.
 *
 * @note  	When retrieving the current operation via flash_get_operation(). There could be more than one operation set at the same time.
 *			E.g. there can be FLASH_STORE_OPERATION and FLASH_ERASE_ERROR set at the same time (if the 
 *			former scheduled erase operation failed and we scheduled a store operation after that).
 */
typedef enum {
	FLASH_NO_OPERATION 					= 0,			/**< Currently no store operation ongoing. */
	FLASH_STORE_OPERATION 				= (1 << 0),		/**< Currently there is an ongoing store operation. */
	FLASH_ERASE_OPERATION 				= (1 << 1),		/**< Currently there is an ongoing erase operation. */
	FLASH_STORE_ERROR					= (1 << 2),		/**< There was an error while storing the former data. */
	FLASH_ERASE_ERROR					= (1 << 3),		/**< There was an error while erasing the former pages. */
} flash_operation_t;






/**@brief   Function for initializing the flash module.
 *
 * @details This functions initializes the underlying fstorage-module with the specified parameters.
 *			The parameters for the fstorage-module are specified in the config-file: sdk_config.h.
 *			
 *
 * @retval  NRF_SUCCESS    		If the module was successfully initialized.
 * @retval  NRF_ERROR_INTERNAL  If there was an error while initializing the fstorage-module.
 */
ret_code_t flash_init(void);





/**@brief   Function for erasing pages in flash in asynchronous/non-blocking/background mode.
 *
 * @details Function uses the fstorage-library to erase pages in flash memory.
 *			This is a non-blocking function. So you can just check the status of the ongoing erase operation
 *			by calling flash_get_erase_operation().
 *			Normally you should always check flash_get_erase_operation() before calling
 *			this function. Because if the erasing operation failed, the mentioned function 
 *			will inform you by returning the flash_erase_operation_t FLASH_ERASE_ERROR.
 *
 * @param[in]   page_num	   	The index of the first page to erase.
 * @param[in]   num_pages		Number of pages to erase.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (erase or store).
 * @retval 	NRF_ERROR_INTERNAL	    	If the module is not initialized or has a false configuration.
 * @retval  NRF_ERROR_INVALID_PARAM		If the specified input parameters are bad.
 */
ret_code_t flash_erase_bkgnd(uint32_t page_num, uint16_t num_pages);


/**@brief   Function for erasing pages in flash in blocking mode.
 *
 * @details Function uses internally flash_erase_bkgnd() for erasing the data
 *			and the flash_get_erase_operation() to wait until the operation has terminated.
 *
 * @param[in]   page_num	   	The index of the first page to erase.
 * @param[in]   num_pages		Number of pages to erase.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (erase or store).
 * @retval 	NRF_ERROR_INTERNAL	    	If the module is not initialized or has a false configuration.
 * @retval  NRF_ERROR_INVALID_PARAM		If the specified input parameters are bad.
 * @retval  NRF_ERROR_TIMEOUT			If the store operation timed out or the operation takes too long.
 */
ret_code_t flash_erase(uint32_t page_num, uint16_t num_pages);





/**@brief   Function for storing data in flash in asynchronous/non-blocking/background mode.
 *
 * @details Function uses the fstorage-library to store data into flash memory. 
 *			The fstorage library possibly splits the data into smaller chunks, 
 *			to have more chances to get stored between BLE events.
 *			This is a non-blocking function. So you can just check the status of the ongoing store operation
 *			by calling flash_get_store_operation().
 *			Normally you should always check flash_get_store_operation() before calling
 *			this function. Because if the store operation failed, the mentioned function 
 *			will inform you by returning the flash_store_operation_t FLASH_STORE_ERROR.
 *	
 * @warning The data to be written to flash has to be kept in memory until the operation has terminated.
 *
 * @param[in]   word_num	   	The 32bit address where to store the data. Starts at 0 .
 * @param[in]   p_words         Pointer to the data to store in flash (32 bit words).
 * @param[in]   length_words    Length of the data to store, in words.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (erase or store).
 * @retval 	NRF_ERROR_INTERNAL	    	If the module is not initialized or has a false configuration.
 * @retval  NRF_ERROR_INVALID_PARAM		If the specified input parameters are bad.
 */
ret_code_t flash_store_bkgnd(uint32_t word_num, const uint32_t* p_words, uint16_t length_words);


/**@brief   Function for storing data in flash in blocking mode.
 *
 * @details Function uses internally flash_store_bkgnd() for storing the data
 *			and the flash_get_store_operation() to wait until the operation has terminated.
 *	
 *
 * @param[in]   word_num	   	The 32bit address where to store the data. Starts at 0 .
 * @param[in]   p_words         Pointer to the data to store in flash (32 bit words).
 * @param[in]   length_words    Length of the data to store, in words.
 *
 * @retval  NRF_SUCCESS             	If the operation was started successfully.
 * @retval  NRF_ERROR_BUSY				If there is already an ongoing operation (erase or store).
 * @retval 	NRF_ERROR_INTERNAL	    	If the module is not initialized or has a false configuration.
 * @retval  NRF_ERROR_INVALID_PARAM		If the specified input parameters are bad.
 * @retval  NRF_ERROR_TIMEOUT			If the store operation timed out or the operation takes too long.
 */
ret_code_t flash_store(uint32_t word_num, const uint32_t* p_words, uint16_t length_words);


/**@brief   Function for retrieving the current store status/operation.
 *
 * @details This function returns the current flash_operation_t (and combinations of them).
 *			E.g. there can be FLASH_STORE_OPERATION and FLASH_ERASE_ERROR set at the same time (if the 
 *			former scheduled erase operation failed and we scheduled a store operation after that).
 *			The application can check the status through this function, 
 *			to decide whether the operation is done, or to reschedule
 *			the operation of the former data/pages because of an error while storing/erasing.
 *
 *
 * @retval FLASH_STORE_NO_OPERATION 	If there is currently no store operation in process.
 * @retval FLASH_STORE_OPERATION		If there is currently a store operation in process.
 * @retval FLASH_ERASE_OPERATION		If there is currently a store operation in process.
 * @retval FLASH_STORE_ERROR			If an error occured while storing the former data. So the application can reschedule the storage of the former data.	
 * @retval FLASH_ERASE_ERROR			If an error occured while erasing the former pages. So the application can reschedule the erase of the former pages.	
 */
flash_operation_t flash_get_operation(void);



/**@brief   Function for reading words from flash.
 *
 *
 * @warning The application must ensure that p_words has space for least length_words.
 *
 * @param[in]   word_num	   	The address of the first word to read.
 * @param[in]  	p_words			Pointer to memory where the words should be stored to.
 * @param[in]  	length_words	Number of words to read.
 *
 * @retval  NRF_SUCCESS         		If the operation was successfully.
 * @retval 	NRF_ERROR_INVALID_PARAM		If the p_words pointer is not in RAM section, the word_num (address) is invalid or p_word is NULL.
 */
ret_code_t flash_read(uint32_t word_num, uint32_t* p_words, uint16_t length_words);


/**@brief   Function for reading number of words in one page.
 *
 * @retval  Number of words in one page.
 */
uint32_t flash_get_page_size_words(void);

/**@brief   Function for reading available number of pages.
 *
 * @retval  Number of available pages.
 */
uint32_t flash_get_page_number(void);


/**@brief   Function for testing the flash module.
 *
 * @retval  0	If selftest failed.
 * @retval  1	If selftest passed.
 */
bool flash_selftest(void);


#endif
