/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"
#include "provisioning.h"
#include "server.h"
#include "hardware/gpio.h"





#define HEARTBEAT_PERIOD_MS 1000
#define APP_AD_FLAGS 0x06

static uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x10, 0xFF,
};

static const uint8_t adv_data_len = sizeof(adv_data);

int le_notification_enabled;
hci_con_handle_t con_handle;
uint8_t ssid;
uint8_t password;

char ssid_word[32];
char password_word[32];

bool connection_status;


static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;

void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));

            // setup advertisements
            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            uint8_t adv_type = 0;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
            assert(adv_data_len <= 31); // ble limitation
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);


            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            le_notification_enabled = 0;
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            att_server_notify(con_handle, ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE, (uint8_t*)&ssid, sizeof(ssid));
            att_server_notify(con_handle, ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE, (uint8_t*)&password, sizeof(password));
            break;
        
        default:
            break;
    }
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(connection_handle);

    // SSID read callbaclk
    if (att_handle == ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)&ssid, sizeof(ssid), offset, buffer, buffer_size);
    }

    // Password read callback
    if (att_handle == ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE){
        return att_read_callback_handle_blob((const uint8_t *)&password, sizeof(password), offset, buffer, buffer_size);
    }

    return 0;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    UNUSED(offset);
    UNUSED(buffer_size);
    
 
    le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    con_handle = connection_handle;
    if (le_notification_enabled) {
        att_server_request_can_send_now_event(con_handle);
        printf("event1\n");
        //This occurs when the client enables notification (the download button on nrf scanner)
    }

    // First characteristic (SSID)
    if (att_handle == ATT_CHARACTERISTIC_b1829813_e8ec_4621_b9b5_6c1be43fe223_01_VALUE_HANDLE){
        att_server_request_can_send_now_event(con_handle);
        printf("event2\n");
        memcpy(ssid_word, buffer, buffer_size);
        //This occurs when the client sends a write request to the ssid characteristic (up arrow on nrf scanner)

    }

    // Second characteristic (Password)
    if (att_handle == ATT_CHARACTERISTIC_410f5077_9e81_4f3b_b888_bf435174fa58_01_VALUE_HANDLE){
        att_server_request_can_send_now_event(con_handle);
        printf("event3\n");
        memcpy(password_word, buffer, buffer_size);
        //This occurs when the client sends a write request to the password characteristic (up arrow on nrf scanner)

    }


    return 0;
}


static void heartbeat_handler(struct btstack_timer_source *ts) {
    static uint32_t counter = 0;
    counter++;
   
    // Invert the led
    static int led_on = true;
    led_on = !led_on;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);

    // Restart timer
    btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

int main() {
    stdio_init_all();

    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch\n");
        return -1;
    }

    l2cap_init();
    sm_init();


    att_server_init(profile_data, att_read_callback, att_write_callback);    

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    // set one-shot btstack timer
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
    btstack_run_loop_add_timer(&heartbeat);

    // turn on bluetooth!
    hci_power_control(HCI_POWER_ON);


    
    // btstack_run_loop_execute is only required when using the 'polling' method (e.g. using pico_cyw43_arch_poll library).
    // This example uses the 'threadsafe background` method, where BT work is handled in a low priority IRQ, so it
    // is fine to call bt_stack_run_loop_execute() but equally you can continue executing user code.

#if 0 // btstack_run_loop_execute() is not required, so lets not use it
    btstack_run_loop_execute();
#else
    // this core is free to do it's own stuff except when using 'polling' method (in which case you should use 
    // btstacK_run_loop_ methods to add work to the run loop.
    


    //wait for both credentials to be passed before attempting connection
    while ((strcmp(ssid_word, "") == 0) || (strcmp(password_word, "") == 0)) {
        sleep_ms(1000);
        printf("%s\n", ssid_word);
        printf("%s\n", password_word);
    }

    //start trying to connect to wifi with BLE credentials
    cyw43_arch_enable_sta_mode();
    printf("connection begin");

    //repeatedly try to connect 
    while (connection_status == false) {
        if (cyw43_arch_wifi_connect_timeout_ms(ssid_word, password_word, CYW43_AUTH_WPA2_AES_PSK, 5000)) { 
            printf("failed to connect.\n");
        } else {
            printf("Connected.\n");
            connection_status = true;
        }
        sleep_ms(5000);
    }

    printf("succesful connection!");

#endif
    return 0;
}
