/* Record frames to a file and read them back.
 *
 * The format is the one the standard logger writes, so a recording made here
 * can be read by tools that have never heard of this repository, and a
 * recording made by those tools can be replayed by this one:
 *
 *     (1696118400.123456) vcan0 123#DEADBEEF
 *     (1696118400.124556) vcan0 1A3##1112233445566778899AABBCCDDEEFF00
 *
 * Replay is what makes a missing frame reproducible. Chapter 11's sequence gap
 * detection is demonstrated by dropping a frame on purpose, and dropping it the
 * same way twice is only possible if the traffic came from a file rather than
 * from a bus that will never repeat itself exactly.
 *
 * Nothing here opens a socket either, so this half is tested on the authoring
 * laptop alongside jn_frame.
 */
#ifndef JN_LOG_H
#define JN_LOG_H

#include "jn_frame.h"

/* Enough for the longest line: the bracketed stamp, an interface name, and a
 * printed frame. */
#define JN_LOG_MAX  256u

/* One frame as a log line, using the frame's own stamp. Returns the length
 * written, not counting the terminator, or a negative jn_err_t. JN_LOG_MAX
 * bytes is always enough for any interface name of sane length. */
int jn_log_line(const jn_frame_t *f, const char *iface, char *out, size_t cap);

/* One log line back into a frame. The interface name is copied into iface when
 * that is not NULL, and ignored when it is. A stamp with more than nine
 * fractional digits is truncated to nanoseconds rather than refused, because
 * some loggers print more. Returns JN_OK or a negative jn_err_t. */
int jn_log_parse(const char *line, jn_frame_t *f, char *iface, size_t ifcap);

/* Write n frames, one per line. Returns n, or a negative jn_err_t. */
int jn_log_write(const char *path, const jn_frame_t *frames, size_t n,
                 const char *iface);

/* Every frame in the file, in order, with the stamps it was given. Blank lines
 * and lines beginning with a hash are skipped, so a recording can be annotated.
 * Returns the number of frames read, or a negative jn_err_t. */
int jn_log_read(const char *path, jn_frame_t *out, size_t cap);

/* Remove every nth frame in place, which is how a gap is made on purpose. The
 * positions removed are written to dropped, so a test can assert that the
 * detector found exactly the gaps that were made and not one more. Returns the
 * number of frames kept, or a negative jn_err_t; every < 2 is refused, because
 * dropping every frame is not a gap. */
int jn_log_drop(jn_frame_t *frames, size_t n, size_t every,
                size_t *dropped, size_t dcap);

#endif /* JN_LOG_H */
