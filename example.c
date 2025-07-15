#include <stdio.h>
#include <wifi_prov_lib.h>


int main(void) {
    start_ble_wifi_provisioning();
    printf("finished provisioning\n");
}