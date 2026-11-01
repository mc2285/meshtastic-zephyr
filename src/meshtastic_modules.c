/* SPDX-FileCopyrightText: Benjamin Cabé <kartben@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include "meshtastic_modules.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(meshtastic, CONFIG_MESHTASTIC_LOG_LEVEL);

static bool packet_is_to_us(const struct meshtastic_packet *packet)
{
	return packet != NULL && packet->to == meshtastic_get_node_id();
}

void meshtastic_dispatch_modules(const struct meshtastic_packet *packet)
{
	struct meshtastic_packet reply = {0};
	const struct meshtastic_module *handler = NULL;
	int ret;

	if (packet == NULL) {
		return;
	}

	STRUCT_SECTION_FOREACH(meshtastic_module, mod) {
		if (mod->on_packet == NULL) {
			continue;
		}

		if ((mod->flags & MESHTASTIC_MODULE_ALL_PACKETS) != 0U ||
		    mod->portnum == packet->portnum) {
			mod->on_packet(packet);
		}
	}

	if (!packet_is_to_us(packet) || !packet->want_response) {
		return;
	}

	STRUCT_SECTION_FOREACH(meshtastic_module, mod) {
		if (mod->portnum != packet->portnum || mod->alloc_reply == NULL) {
			continue;
		}

		ret = mod->alloc_reply(packet, &reply);
		if (ret == -ENOENT) {
			continue;
		}

		handler = mod;
		break;
	}

	if (handler == NULL) {
		LOG_DBG("want_response on port %u but no handler", (unsigned int)packet->portnum);
		return;
	}

	if (ret < 0) {
		LOG_WRN("Module '%s' alloc_reply failed (%d)", handler->name, ret);
		return;
	}

	meshtastic_packet_set_reply_to(&reply, packet);

	LOG_INF("Module '%s' sending want_response reply", handler->name);
	/*
	 * Random jitter before the first attempt: several nodes can be
	 * answering each other's want_response requests in the same instant
	 * (paired exchanges triggered by the same broadcasts), and without
	 * jitter their replies start together and collide.  The jitter lets
	 * LBT serialize the pair.
	 */
	k_sleep(K_MSEC(50U + (sys_rand32_get() % 451U)));
	ret = meshtastic_send_packet(&reply, K_NO_WAIT);
	if (ret < 0) {
		/*
		 * The first attempt most likely lost the LBT race: the
		 * channel was busy and the transmit was dropped without
		 * reaching the air.  Back off briefly and try once more;
		 * the reply picks a fresh packet id on the retry.
		 */
		k_sleep(K_MSEC(200U + (sys_rand32_get() % 401U)));
		ret = meshtastic_send_packet(&reply, K_NO_WAIT);
	}
	if (ret < 0) {
		LOG_WRN("Reply send failed (%d)", ret);
	}
}
