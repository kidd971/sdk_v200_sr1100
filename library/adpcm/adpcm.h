/** @file  adpcm.h
 *  @brief Types definitions and functions prototypes for ADPCM compression.
 *
 *  This implementation is based on the algorithm described in
 *  "Recommended Practices for Enhancing Digital Audio Compatibility in
 *  Multimedia Systems" by the IMA Digital Audio Focus and Technical
 *  Working Groups, revision 3.0.
 *  Reference: http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf
 *
 *  This implementation uses a state type to hold the encoder and
 *  decoder state information, thus allowing multiple instances of
 *  each to coexist.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef ADPCM_H_
#define ADPCM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include <stdint.h>

/* TYPES **********************************************************************/
typedef struct state_variable {
        int16_t predicted_sample;
        uint8_t index;
} __attribute__((packed)) state_variable_t;

typedef union adpcm_state {
    state_variable_t state;
    int8_t byte_array[sizeof(state_variable_t)];
} __attribute__((packed)) adpcm_state_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize ADPCM state.
 *
 *  @param[out]  state ADPCM state.
 */
void adpcm_init_state(adpcm_state_t *state);

/** @brief Encode 16-bit PCM sample using ADPCM compression.
 *
 *  Excerpt from "Recommended Practices for Enhancing Digital Audio Compatibility in
 *  Multimedia Systems" by the IMA Digital Audio Focus and Technical Working Groups, revision 3.0.
 *  Reference: http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf
 *  Note: the following text shows adjusted variables name which differ from the original text.
 *
 *      The following algorithm assumes original_sample is a 16-bit two’s complement variable.
 *      The variable new_sample is the resulting 4-bit ADPCM sample.
 *      The algorithm finds the difference between the original_sample and predicted_sample, the
 *      output of its predictor. This difference is then quantized down to a 4-bit new_sample, using
 *      step_size. The 4-bit new_sample has a sign-magnitude format. After new_sample has been
 *      calculated, it is uncompressed using the same quantization step_size to obtain a linear
 *      difference identical to that calculated by the decompressor. In order to correct
 *      for truncation errors in the quantization, ½ is effectively added to new_sample during
 *      the expansion. This difference is added to predicted_sample to form a prediction for the
 *      next sequential original_sample. newSample is used to adjust an index into the
 *      step_size_table. This index points to a new step_size in the step_size_table.
 *      predicted_sample, step_size, and index must be static variables between samples.
 *
 *  @param[in]  original_sample  16-bit PCM sample.
 *  @param[out] state            Internal ADPCM encoder state.
 *  @retval 4-bit ADPCM sample.
 */
uint8_t adpcm_encode(int32_t original_sample, adpcm_state_t *state);

/** @brief Decode 4-bit ADPCM sample into 16-bit PCM sample.
 *
 *  Excerpt from "Recommended Practices for Enhancing Digital Audio Compatibility in
 *  Multimedia Systems" by the IMA Digital Audio Focus and Technical Working Groups, revision 3.0.
 *  Reference: http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf
 *  Note: the following text shows adjusted variables name which differ from the original text.
 *
 *      The following algorithm assumes original_sample is a 4-bit ADPCM sample. The variable
 *      new_sample is the resulting 16-bit two’s complement variable.
 *      original_sample is uncompressed using a quantization step_size to obtain a linear
 *      difference. In order to correct for truncation errors in the quantization, ½ is effectively
 *      added to original_sample during the expansion. This difference is added to
 *      predicted_sample to form a linear new_sample. original_sample is used to adjust an
 *      index into the step_size_table. This index points to a new step_size in the step_size_table.
 *      new_sample, step_size, and index must be static variables between samples.
 *
 *  @param[in]  original_sample   4-bit ADPCM sample.
 *  @param[out] state             Internal ADPCM decoder state.
 *  @retval 16-bit PCM sample.
 */
int16_t adpcm_decode(uint8_t original_sample, adpcm_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* ADPCM_H_ */
