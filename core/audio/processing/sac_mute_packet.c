/** @file  sac_mute_packet.c
 *  @brief Processing stage used to avoid sending packets full of samples with a numerical value of zero.
 *
 *  @note This processing stage should be the last processing stage on an audio transmitting pipeline
 *        and the first processing stage on an audio receiving pipeline.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_mute_packet.h"
#include <string.h>

/* PUBLIC FUNCTIONS ***********************************************************/
uint16_t sac_mute_packet_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                 uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;

    uint8_t i;
    uint8_t zeros;

    *status = SAC_OK;

    if (instance == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return 0;
    }

    if (((sac_mute_packet_instance_t *)instance)->is_tx) {
        zeros = 0;
        /* look for a packet containing only zeros */
        for (i = 0; i < size; i++) {
            if (data_in[i] == 0) {
                zeros++;
            }
        }
        if (zeros == size) {
            /* packet is muted */
            data_out[0] = size;
            return 1;
        } else {
            /* packet not muted */
            return 0;
        }
    } else {
        if (size == 1) {
            /* reconstruct muted packet */
            memset(data_out, 0, data_in[0]);
            return data_in[0];
        } else {
            return 0;
        }
    }
}
