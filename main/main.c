/**************************************************************************************************/
/** @file     main.c
 *  @brief    WiFi station Example
 *  @details  x
 *
 *  @author   Justin Reina, Firmware Engineer
 *  @created  3/21/25
 *  @last rev 3/22/25
 *
 *
 *  @notes    Work in progress!!
 *
 *  @section    Opens
 *      unroll commands
 *		step in the arduino code
 *		port it over
 *		make it work
 *
 *	@section  	Console Configuration
 *		COM4 @ 115200 bps, 8 data bits, 1 stop bit, no parity & no flow ctrl
 *
 *  @section    Legal Disclaimer
 *      © 2025 Justin Reina, All rights reserved. All contents of this source file and/or any other
 *      related source files are the explicit property of Justin Reina. Do not distribute.
 *      Do not copy.
 */
/**************************************************************************************************/

//************************************************************************************************//
//                                            INCLUDES                                            //
//************************************************************************************************//

//Standard Library Includes
#include <string.h>

//FreeRTOS Includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

//Library Includes
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

//SDK Includes
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"


//************************************************************************************************//
//                                        DEFINITIONS & TYPES                                     //
//************************************************************************************************//

//-----------------------------------------  Definitions -----------------------------------------//

//Network Credentials
#define EXAMPLE_ESP_WIFI_SSID      			"MCC Corp"
#define EXAMPLE_ESP_WIFI_PASS      			"Metro2016"
#define EXAMPLE_ESP_MAXIMUM_RETRY  			(5)

//Network Config
#define ESP_WIFI_SAE_MODE 					WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER 				CONFIG_ESP_WIFI_PW_ID
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD 	WIFI_AUTH_WPA2_PSK

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT 					BIT0
#define WIFI_FAIL_BIT      					BIT1


//************************************************************************************************//
//                                            VARIABLES                                           //
//************************************************************************************************//

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

//Project variables
static const char *TAG = "wifi station";

static int s_retry_num = 0;


//************************************************************************************************//
//                                         PRIVATE FUNCTIONS                                      //
//************************************************************************************************//

/**************************************************************************************************/
/** @fcn        static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
 *  @brief      x
 *  @details    x
 *
 *  @param    [in]  (void *) arg - x
 *  @param    [in[  (esp_event_base_t) event_base - x
 *  @param    [in]  (int32_t) event_id - x
 *  @param    [in]  (void *) event_data - x
 */
/**************************************************************************************************/
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	
    if((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START)) {
		
        esp_wifi_connect();
        
    } else if((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED)) {
		
        if(s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
			
            esp_wifi_connect();
            
            s_retry_num++;
            
            ESP_LOGI(TAG, "retry to connect to the AP");
            
        } else {
			
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            
        }
        
        ESP_LOGI(TAG,"connect to the AP fail");
        
    } else if((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
		
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        
        s_retry_num = 0;
        
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    
    return;
}


/**************************************************************************************************/
/** @fcn        void wifi_init_sta(void)
 *  @brief      Initialize WiFi connection to AP
 *  @details    x
 *
 *	@section  	Opens
 */
/**************************************************************************************************/
void wifi_init_sta(void) {
	
	//Locals
	esp_err_t stat;									/* sdk status response 						  */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;


	//Init Event
    s_wifi_event_group = xEventGroupCreate();
	
	//Init NetIf
	stat = esp_netif_init();

	//Safety
    ESP_ERROR_CHECK(stat);

	stat = esp_event_loop_create_default();

	//Safety
    ESP_ERROR_CHECK(stat);
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    
    stat = esp_wifi_init(&cfg);
    
    //Safety
    ESP_ERROR_CHECK(stat);
    
    stat = esp_event_handler_instance_register(
											   WIFI_EVENT,                    
											   ESP_EVENT_ANY_ID,             
											   &event_handler,       
											   NULL,  
											   &instance_any_id
											  );
    //Safety
    ESP_ERROR_CHECK(stat);
                                                        
    stat = esp_event_handler_instance_register(
											   IP_EVENT,          
											   IP_EVENT_STA_GOT_IP,                        
											   &event_handler,    
											   NULL,       
											   &instance_got_ip
											  );
	//Safety
	ESP_ERROR_CHECK(stat);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards 
             * (password len => 8). If you want to connect the device to depr WEP/WPA networks, 
             * Please set the threshold value to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set 
             * the pswd w\length and format matching to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e        = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    
    stat = esp_wifi_set_mode(WIFI_MODE_STA);
    
    //Safety
    ESP_ERROR_CHECK(stat);
    
    stat = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    
    //Safety
    ESP_ERROR_CHECK(stat);
    
    stat = esp_wifi_start();

	//Safety
    ESP_ERROR_CHECK(stat);


	//Notice
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed 
     * for the max # of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
							               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
								           pdFALSE,
								           pdFALSE,
								           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which 
     * event actually happened */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    
    return;
}


/**************************************************************************************************/
/** @fcn        void app_main(void)
 *  @brief      x
 *  @details    x
 */
/**************************************************************************************************/
extern int test_jmr;
void app_main(void) {
	
    //Initialize NVS
    esp_err_t ret;									/* sdk status response 						  */
    
    //Init
    ret = nvs_flash_init();
    
    //Safety
    if((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
		
      ESP_ERROR_CHECK(nvs_flash_erase());
      
      ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    
    wifi_init_sta();

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA Prepared. %d", test_jmr);

    
    return;
}
