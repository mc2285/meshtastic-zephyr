/* SPDX-FileCopyrightText: Benjamin Cabé <kartben@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

/**
 * @file meshtastic_channels.h
 * @brief Internal channel initialization and device policy helpers.
 *
 * The channel table API itself is public: see
 * <zephyr/meshtastic/channels.h>.  This header only holds what is not
 * meaningful for applications: boot-time table initialization and the
 * runtime device-role / rebroadcast policy.
 */

#ifndef ZEPHYR_SUBSYS_MESHTASTIC_CHANNELS_H_
#define ZEPHYR_SUBSYS_MESHTASTIC_CHANNELS_H_

#include <meshtastic/config.pb.h>

#include <zephyr/meshtastic/channels.h>
#include <zephyr/meshtastic/meshtastic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize all channel slots to stack defaults.
 *
 * Slot 0 becomes the primary LongFast channel with
 * @ref meshtastic_default_psk; remaining slots are disabled placeholders with
 * hashes computed via @ref meshtastic_channels_get_hash rules.
 *
 * @return 0 on success (always succeeds today).
 */
int meshtastic_channels_init_defaults(void);

/**
 * @brief Initialize channels from application Kconfig / @ref meshtastic_config.
 *
 * Calls @ref meshtastic_channels_init_defaults first, then overwrites the
 * primary channel name and PSK from @p cfg when non-NULL. Also refreshes the
 * runtime state's primary hash, PSK, and name.
 *
 * @param cfg Build-time configuration, or @c NULL to keep defaults only.
 * @return 0 on success, or a negative errno from the defaults path.
 */
int meshtastic_channels_init_from_config(const struct meshtastic_config *cfg);

/**
 * @brief Pick the channel index to use when sending a packet.
 *
 * Resolution order:
 * 1. Use @p channel_index if that slot is enabled.
 * 2. Else, if @p wire_hash is non-zero, find the first slot matching the hash.
 * 3. Else, if NodeDB is enabled and @p dest is a known unicast peer, use the peer's stored channel
 *    when valid.
 * 4. Else fall back to the primary channel index.
 *
 * @param dest Destination node ID (used for NodeDB lookup).
 * @param channel_index Requested index from the caller (may be invalid).
 * @param wire_hash Header hash from an incoming packet when replying, or 0.
 * @return Resolved slot index for TX crypto and @c Data.channel field.
 */
uint8_t meshtastic_channels_resolve_send_index(uint32_t dest, uint8_t channel_index,
                                              uint8_t wire_hash);

/**
 * @brief Current device role (CLIENT, ROUTER, …).
 */
meshtastic_Config_DeviceConfig_Role meshtastic_device_role(void);

/**
 * @brief Current rebroadcast mode filter.
 */
meshtastic_Config_DeviceConfig_RebroadcastMode meshtastic_rebroadcast_mode(void);

/**
 * @brief Override the runtime device role.
 *
 * @param role New @c DeviceConfig.role value.
 */
void meshtastic_set_device_role(meshtastic_Config_DeviceConfig_Role role);

/**
 * @brief Override the runtime rebroadcast mode.
 *
 * @param mode New @c DeviceConfig.rebroadcast_mode value.
 */
void meshtastic_set_rebroadcast_mode(meshtastic_Config_DeviceConfig_RebroadcastMode mode);

/**
 * @brief Whether this node should relay packets originated by others.
 *
 * Rebroadcasting is suppressed for @c CLIENT_MUTE role or when rebroadcast
 * mode is @c NONE.
 */
bool meshtastic_is_rebroadcaster(void);

/**
 * @brief Whether a packet from @p from may be decoded/processed under policy.
 *
 * When rebroadcast mode is not @c KNOWN_ONLY, always returns @c true. In
 * @c KNOWN_ONLY mode, returns @c true only if NodeDB contains @p from (when
 * NodeDB is compiled in); otherwise returns @c false.
 *
 * @param from Source node ID from the packet header.
 * @return @c true if the sender is allowed under the current rebroadcast
 *         policy.
 */
bool meshtastic_decode_known_only(uint32_t from);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_MESHTASTIC_CHANNELS_H_ */
