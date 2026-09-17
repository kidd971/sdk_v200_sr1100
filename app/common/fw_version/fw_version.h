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
#define FW_VERSION_SDK  "v2.4.1"

/** @brief Release suffix, INCLUDING its separator. Always "_rcN".
 *
 *  Carries the underscore itself rather than having the separator live in the concatenation
 *  below, so the suffix is one value a -D can replace outright without anything having to
 *  know how it joins to the half in front of it.
 *
 *  One shape, for every package including the ones that ship -- decided 2026-09-17. It used
 *  to be a scheme: "_rcN" while a package was a candidate, "" once it shipped, ".1" for a
 *  patch on top of something already out. Three spellings of one line meant a version string
 *  had to be interpreted before it could be compared, and the spelling that mattered most --
 *  the bare release -- was the one that read as a field gone missing. So the number just
 *  keeps counting: bump N when cutting a package, whatever the package is for, and keep the
 *  value equal to the bin/<name>/ suffix so the banner, the MANIFEST and the folder agree.
 *
 *  The number is not zero-padded -- "_rc3", not "_rc03" -- so it stays the number it is and
 *  widens on its own at "_rc10". The v2.4.0 line used the padded spelling; a line that has
 *  already shipped keeps whatever it shipped as, so the two forms coexist in the tags and in
 *  the older notes. Everything from v2.4.1 on is unpadded.
 *
 *  Overridable per build so a preset can stamp it without editing this file. Pass it through
 *  CMake WITHOUT quotes -- -DFW_VERSION_RELEASE=_rc3 -- because the forward in the root
 *  CMakeLists.txt adds them; quoting it there lands two sets of quotes on the compiler line.
 */
#ifndef FW_VERSION_RELEASE
#define FW_VERSION_RELEASE  "_rc3"
#endif

/** @brief What every version print and version query answers: "v2.4.1_rc3".
 *
 *  One string, deliberately, because the alternative has already happened here: the two
 *  halves lived in separate macros with separate rules about when to bump them, and the
 *  console ended up reporting the SDK tag while the package was on a different number
 *  entirely. A log pasted into a bug report then names a version nobody cut.
 *
 *  Underscore rather than a space so the whole thing is one token: it shares a line with
 *  data in the boot banner and with the AT responses, and anything that splits on whitespace
 *  -- a log grep, a host parser, a person copying a field -- would otherwise take half of it.
 *  The git tag and the bin/<tag>/ directory use the same spelling.
 *
 *  It is a convenience label and can still go stale. The __DATE__/__TIME__ printed beside it
 *  is what actually identifies a binary, and the MANIFEST's git commit is what actually
 *  identifies the source.
 */
#define FW_VERSION_STRING  FW_VERSION_SDK FW_VERSION_RELEASE

/** @brief The SDK line without its dots, for places where the version shares a line with data.
 *
 *  Kept beside FW_VERSION_SDK rather than derived from it -- the preprocessor cannot strip the
 *  dots -- so the two forms of the same number sit together and cannot be changed one without
 *  seeing the other.
 */
#define FW_VERSION_SDK_COMPACT  "v241"

/** @brief One whitespace-free token: "v241_rc3".
 *
 *  For the statistics lines, which are read by eye in a terminal and split on whitespace by
 *  everything else. The release half is the same FW_VERSION_RELEASE the long form uses, so
 *  cutting a package still touches one place.
 */
#define FW_VERSION_COMPACT  FW_VERSION_SDK_COMPACT FW_VERSION_RELEASE

#ifdef __cplusplus
}
#endif

#endif /* FW_VERSION_H_ */
