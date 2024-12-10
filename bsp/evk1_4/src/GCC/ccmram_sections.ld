/* This file allows the user to add functions in the CCMRAM.
 *
 * The location of this file must be added with the -L argument in the
 * cmake variable CMAKE_EXE_LINKER_FLAGS
 *
 * set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/absolute/path/to/this/file")
 *
 * Relocating functions to CCMRAM is done by specifying the input file, the section and/or the functions name as described in the GNU LD documentation
 * (https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_3.html#SEC19)
 *
 * Here are some examples:
 *     Relocate the content of the "text" section from files in the WPS library (libwps.a), excluding the wps.c file (wps.c.obj).
 *     libwps.a:(EXCLUDE_FILE(wps.c.obj) .text)
 *     Relocate any content from the "text" containing the word "radio" in its name from all files (note the wildcard "*" at the beginning).
 *     *(.text.*radio*)
 */
