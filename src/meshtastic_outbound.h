/* SPDX-FileCopyrightText: Benjamin Cabé <kartben@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ZEPHYR_SUBSYS_MESHTASTIC_OUTBOUND_H_
#define ZEPHYR_SUBSYS_MESHTASTIC_OUTBOUND_H_

#include <stdint.h>

#include <zephyr/kernel.h>

#include "meshtastic_core.h"

#ifdef __cplusplus
extern "C" {
#endif

int meshtastic_outbound_init(void);

/*
 * Queue a wire frame for transmission.  Returns 0 when queued, -ENOMSG if the
 * queue is full.  Does not block on the radio driver.
 */
int meshtastic_radio_send_wire(uint8_t *pkt, uint32_t pkt_len);

/*
 * Queue a wire frame the same way, but mark it as expendable: if the channel
 * is busy when its turn comes, the frame is skipped instead of backing off
 * and holding the radio deaf through the retry window.  Used for relays,
 * where hearing the ongoing transmission matters more than adding another
 * copy of an already-flooded packet.
 */
int meshtastic_radio_send_wire_best_effort(uint8_t *pkt, uint32_t pkt_len);

/*
 * Queue a wire frame and block until the outbound worker completes the
 * transmission or @p timeout expires (-EAGAIN).
 */
int meshtastic_radio_send_wire_wait(const uint8_t *pkt, uint32_t pkt_len, k_timeout_t timeout);

/* Driver-level TX; only called from the outbound worker thread. */
int meshtastic_radio_send_wire_now(uint8_t *pkt, uint32_t pkt_len, bool yield_on_busy);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_MESHTASTIC_OUTBOUND_H_ */
