#ifndef USBLINK_CX2_H
#define USBLINK_CX2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct usblink_cx2_state {
	bool handshake_complete;
	uint16_t seqno;
} usblink_cx2_state;

bool usblink_cx2_handle_packet(const uint8_t *data, uint16_t size);
bool usblink_cx2_send_navnet(const uint8_t *data, uint16_t size);
void usblink_cx2_reset();

#ifdef __cplusplus
}
#endif

#endif // USBLINK_CX2_H
