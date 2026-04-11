#include "esp_ipc.h"

static volatile uint8_t *ipc_base = (volatile uint8_t *)IPC_BASE_ADDR;

void ipc_write(uint8_t offset, uint8_t value) {
    ipc_base[offset] = value;
}

uint8_t ipc_read(uint8_t offset) {
    return ipc_base[offset];
}

void ipc_send_command(uint8_t cmd, uint8_t len, uint8_t *data) {
    ipc_write(0, cmd);

    for (uint8_t i = 0; i < len; i++) {
        ipc_write(2 + i, data[i]);
    }

    ipc_write(1, len);
}

uint8_t ipc_get_response(uint8_t *len, uint8_t *data, uint8_t max_len) {
    uint8_t status = ipc_read(0);
    *len = ipc_read(1);

    uint8_t read_len = (*len > max_len) ? max_len : *len;

    for (uint8_t i = 0; i < read_len; i++) {
        data[i] = ipc_read(2 + i);
    }

    return status;
}

uint8_t ipc_ping(uint8_t test_byte) {
    uint8_t data[1] = {test_byte};
    ipc_send_command(CMD_PING, 1, data);

    uint8_t resp[254];
    uint8_t len;
    uint8_t status = ipc_get_response(&len, resp, 254);

    if (status == 0 && len >= 1 && resp[0] == test_byte) {
        return 0;
    }
    return 0xFF;
}