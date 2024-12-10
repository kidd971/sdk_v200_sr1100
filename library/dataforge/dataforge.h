/** @file  dataforge.h
 *  @brief Pseudo random data generator/validator with the help of a CRC and data pattern generator/validator.
 *         Often use to validate application.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PSEUDO_DATA_H_
#define PSEUDO_DATA_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Dataforge sequence number match status.
 */
typedef enum {
    DATAFORGE_MATCHING_SEQ     = 0,  /*! Sequence numbers matched */
    DATAFORGE_NON_MATCHING_SEQ = 1,  /*! Sequence numbers do not match */
    DATAFORGE_DUPLICATE_SEQ    = 2,  /*! Sequence numbers are duplicate */
} dataforge_seq_status_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Fill an array with pseudo generated data and CRC
 *
 *  @param[in] data     Pointer to array to put data.
 *  @param[in] size     Size of array.
 *  @param[in] seq_num  Sequence number of the generated data.
 */
void dataforge_generate_pseudo(uint8_t *data, size_t size, uint8_t seq_num);

/** @brief Validate the CRC of a packet received generated with pseudo data.
 *
 *  @param[in] data  Pointer to array to put data.
 *  @param[in] size  Size of array.
 *
 *  @return  True if CRC is match, False otherwise.
 */
bool dataforge_validate_pseudo_crc(uint8_t *data, size_t size);

/** @brief Validate if CRC is present.
 *
 *  @param[in] size  Size of payload received.
 *
 *  @return  True if CRC is present, False otherwise.
 */
bool dataforge_is_pseudo_crc_populated(size_t size);

/** @brief Validate whether the data contains the correct next sequence number.
 *
 *  @param[in]  data        Data to validate.
 *  @param[in]  seq_num     The current sequence number value.
 *
 *  @return  The status code to report sequence number match status.
 */
dataforge_seq_status_t dataforge_validate_seq_num(uint8_t *data, uint8_t seq_num);

/** @brief Update the sequence number to the next value.
 *
 *  @param[in] seq_num  The current sequence number value.
 *
 *  @return  The incremented sequence number value.
 */
uint8_t dataforge_increment_seq_num(uint8_t seq_num);

/** @brief Return the sequence number value from a payload.
 *
 *  @param[in] payload  The payload.
 *
 *  @return  The sequence number value.
 */
uint8_t dataforge_extract_seq_num(uint8_t *payload);

/** @brief Fill payload by repeating payload data array pattern.
 *
 *  @param[in] payload             Payload buffer.
 *  @param[in] payload_data        Payload data pattern buffer.
 *  @param[in] payload_size        Payload size.
 *  @param[in] payload_data_count  Payload data pattern count.
 */
void dataforge_generate_pattern(char *payload, uint32_t *payload_data, uint8_t payload_size, uint8_t payload_data_count);

/** @brief Validate payload of repeating payload data array pattern.
 *
 *  @param[in] payload       Payload buffer.
 *  @param[in] pattern       Expected payload data pattern.
 *  @param[in] payload_size  Payload size.
 *  @param[in] pattern_size  Payload data pattern size.
 *
 *  @return  True if both pattern matches, false otherwise.
 */
bool dataforge_validate_pattern(uint8_t *payload, uint32_t *pattern, uint8_t payload_size, uint8_t pattern_size);

#ifdef __cplusplus
}
#endif

#endif /* PSEUDO_DATA_H_ */

