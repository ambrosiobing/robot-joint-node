/* stamp.h: the one line that says which build is running.
 *
 * The definition is generated into build/stamp.c by tools/gen_stamp.py at every
 * build and is deliberately not committed, because its whole purpose is to
 * differ between two builds of the same source.
 *
 * Why an image on this bench needs one at all: the NUCLEO-H7A3ZI-Q is shared
 * between volumes, so whatever was flashed last is what is on the part. An
 * application that only names itself cannot tell a new image from the previous
 * one, which has already cost two round trips elsewhere on this bench when a
 * skipped flash printed a banner identical to the expected one character for
 * character.
 */
#ifndef JOINT_STAMP_H
#define JOINT_STAMP_H

/* `built <weekday> <day> <month> <year>, <hh:mm:ss> from <commit> tree <state>`
 *
 * Field names rather than bare values, so that a capture found later can be
 * attributed to the image that produced it. `tree` is `clean`, or a count of
 * modified and untracked files: untracked is counted because every file in a
 * new chapter starts untracked and that is the half `git describe --dirty`
 * reports as clean.
 */
const char *build_stamp(void);

#endif /* JOINT_STAMP_H */
