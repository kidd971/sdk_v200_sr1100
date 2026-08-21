/** @file  fw_version.h
 *  @brief Single source of truth for the firmware version printed and reported.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef FW_VERSION_H_
#define FW_VERSION_H_

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/** @brief SDK line this firmware is built from. */
#define FW_VERSION_SDK  "v2.4.0"

/** @brief Release candidate / package tag within that line.
 *
 *  Bump when cutting a package, and keep it equal to the bin/<tag>/ directory name so the
 *  banner, the MANIFEST and the folder all agree. Overridable per build
 *  (-DFW_VERSION_RELEASE=\"rc02\") so a preset can stamp it without editing this file.
 */
#ifndef FW_VERSION_RELEASE
#define FW_VERSION_RELEASE  "rc01"
#endif

/** @brief What every version print and version query answers: "v2.4.0 rc01".
 *
 *  One string, deliberately, because the alternative has already happened here: the two
 *  halves lived in separate macros with separate rules about when to bump them, and the
 *  console ended up reporting the SDK tag while the package was on a different number
 *  entirely. A log pasted into a bug report then names a version nobody cut.
 *
 *  It is a convenience label and can still go stale. The __DATE__/__TIME__ printed beside it
 *  is what actually identifies a binary, and the MANIFEST's git commit is what actually
 *  identifies the source.
 */
#define FW_VERSION_STRING  FW_VERSION_SDK " " FW_VERSION_RELEASE

#ifdef __cplusplus
}
#endif

#endif /* FW_VERSION_H_ */
