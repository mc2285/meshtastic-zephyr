/* SPDX-FileCopyrightText: Benjamin Cabé <kartben@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

/**
 * @file
 * @brief Meshtastic channel table public API.
 *
 * Exposes the runtime channel slot table: inspecting slots, reading the
 * primary channel, replacing slots, and changing a slot's pre-shared key
 * at runtime.  The generated @c meshtastic/channel.pb.h headers must be on
 * the include path; the module publishes them when
 * @kconfig{CONFIG_MESHTASTIC} is enabled.
 */

#ifndef ZEPHYR_INCLUDE_MESHTASTIC_CHANNELS_H_
#define ZEPHYR_INCLUDE_MESHTASTIC_CHANNELS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <meshtastic/channel.pb.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of channel slots in the local table.
 *
 * Fixed by the Meshtastic protobuf schema; indices are always in
 * @c [0, MESHTASTIC_MAX_CHANNELS).
 */
#define MESHTASTIC_MAX_CHANNELS 8U

/**
 * @brief Normalized pre-shared key material for a channel slot.
 *
 * Produced by @ref meshtastic_channels_get_key and
 * @ref meshtastic_channels_primary_key after expanding single-byte "short PSK"
 * indices and inheriting the primary key for secondary channels with an empty
 * PSK. A length of zero means cleartext (no AES) for that slot.
 */
struct meshtastic_channel_key {
	/** Expanded AES-128 (16 bytes) or AES-256 (32 bytes) key bytes. */
	uint8_t bytes[32];
	/** Effective key length after short-PSK expansion; @c 0 for cleartext. */
	size_t len;
};

/**
 * @brief Return the fixed channel table capacity.
 *
 * @return Always @ref MESHTASTIC_MAX_CHANNELS.
 */
uint8_t meshtastic_channels_count(void);

/**
 * @brief Index of the slot currently designated PRIMARY.
 *
 * @return Channel index in @c [0, MESHTASTIC_MAX_CHANNELS).
 */
uint8_t meshtastic_channels_primary_index(void);

/**
 * @brief Wire hash of the primary channel slot.
 */
uint8_t meshtastic_channels_primary_hash(void);

/**
 * @brief Display name of the primary channel slot.
 *
 * @return Same rules as @ref meshtastic_channels_get_name for the primary
 *         slot.
 */
const char *meshtastic_channels_primary_name(void);

/**
 * @brief Normalized AES key for the primary channel slot.
 *
 * @param key Output key buffer (must not be @c NULL).
 * @return 0 on success, or negative errno from @ref meshtastic_channels_get_key.
 */
int meshtastic_channels_primary_key(struct meshtastic_channel_key *key);

/**
 * @brief Read-only view of a channel slot.
 *
 * @param index Slot index.
 * @return Pointer to the internal @c meshtastic_Channel, or @c NULL if
 *         @p index is out of range.  The pointer is stable until the slot is
 *         replaced.
 */
const meshtastic_Channel *meshtastic_channels_get(uint8_t index);

/**
 * @brief Human-readable channel name for a slot.
 *
 * Empty or unset names map to @c LongFast for slot 0 semantics. The returned
 * pointer is stable until the slot is modified.
 *
 * @param index Slot index.
 * @return Channel name string, or @c "" if @p index is out of range.
 */
const char *meshtastic_channels_get_name(uint8_t index);

/**
 * @brief Return the cached wire hash for a channel slot.
 *
 * The hash is the XOR of the channel name bytes and key bytes; it appears in
 * the LoRa packet header and selects the decryption key on receive.
 *
 * @param index Slot index.
 * @return Cached hash, or @c 0 if @p index is invalid.
 */
uint8_t meshtastic_channels_get_hash(uint8_t index);

/**
 * @brief Obtain the normalized AES key for a channel slot.
 *
 * Expands 1-byte short PSK indices, falls back to the primary key for
 * SECONDARY slots with an empty PSK, and leaves @p key->len at 0 for cleartext
 * PRIMARY channels. A key that is neither AES-128 nor AES-256 sized is
 * zero-padded up to the next size, as upstream does.
 *
 * @param index Slot index.
 * @param key Output key buffer (must not be @c NULL).
 * @return 0 on success, @c -EINVAL for bad arguments, @c -ENOENT if the slot
 *         is disabled or missing settings.
 */
int meshtastic_channels_get_key(uint8_t index, struct meshtastic_channel_key *key);

/**
 * @brief Test whether a slot's cached hash matches an on-air header hash.
 *
 * @param index Slot to test.
 * @param wire_hash Hash byte from the received packet header.
 * @return @c true if the slot hash equals @p wire_hash and @p index is valid.
 */
bool meshtastic_channels_decrypt_for_hash(uint8_t index, uint8_t wire_hash);

/**
 * @brief Replace a channel slot and refresh derived state.
 *
 * Copies @p channel into the slot, assigns @c index, demotes any other PRIMARY
 * if the new slot is PRIMARY, recomputes the wire hash, and updates the
 * runtime state when the primary slot changes.
 *
 * To persist the change across reboots as well, follow this call with
 * @c meshtastic_config_store_set_channel (internal API; the shell's
 * @c meshtastic channel set command does both).
 *
 * @param index Slot to update.
 * @param channel Protobuf channel descriptor (must not be @c NULL).
 * @return 0 on success, or @c -EINVAL for invalid arguments.
 */
int meshtastic_channels_set_slot(uint8_t index, const meshtastic_Channel *channel);

/**
 * @brief Change only the pre-shared key of a channel slot.
 *
 * Updates the slot's @c settings.psk in place, keeping name, role, and MQTT
 * flags. Accepted key forms:
 *
 * - @p psk_len == 0: cleartext channel (no AES).
 * - @p psk_len == 1: single-byte short-PSK shorthand (@c 1 selects the
 *   Meshtastic default key; upstream accepts @c 0..10).
 * - @p psk_len == 16 or 32: raw AES-128/AES-256 key bytes.
 *
 * @param index Slot to update.
 * @param psk Key bytes (may be @c NULL only when @p psk_len is 0).
 * @param psk_len Key length in bytes.
 * @return 0 on success, @c -EINVAL for invalid arguments or unsupported
 *         lengths.
 */
int meshtastic_channels_set_psk(uint8_t index, const uint8_t *psk, size_t psk_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MESHTASTIC_CHANNELS_H_ */
