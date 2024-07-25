#ifndef _CRC_H
#define _CRC_H
#include <stdint.h>

namespace crc {

/// @brief Computes the CRC-16 checksum.
/// @param data data pointer to the input data.
/// @param len len Length of the input data in bytes.
/// @return the computed CRC-16 checksum.
uint16_t crc16(const uint8_t* data, int len);

/// @brief Computes the CRC-8 checksum.
/// @param data data pointer to the input data.
/// @param len len Length of the input data in bytes.
/// @return the computed CRC-8 checksum.
uint8_t crc8(const uint8_t* data, int len);

}  // namespace crc
#endif