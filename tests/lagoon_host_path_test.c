#include "lagoon_host_path.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

int main(void) {
  /* A Linux/macOS launcher may legitimately receive either character in an
   * owner-selected relative save directory. Neither makes it absolute. */
  CHECK(!LagoonHostPath_IsAbsolute("C:my-saves",0));
  CHECK(!LagoonHostPath_IsAbsolute("C:/my-saves",0));
  CHECK(!LagoonHostPath_IsAbsolute("\\my-saves",0));
  CHECK(!LagoonHostPath_IsSeparator('\\',0));
  CHECK(LagoonHostPath_IsAbsolute("/home/player/Saved Games",0));
  CHECK(LagoonHostPath_IsAbsolute("/Users/player/Saved Games",0));
  CHECK(LagoonHostPath_IsAbsolute("/home/player/a\\b",0));
  CHECK(LagoonHostPath_IsSeparator('/',0));
  /* Windows drive-relative paths are not rooted, while rooted drive/UNC paths
   * keep their existing absolute treatment. */
  CHECK(!LagoonHostPath_IsAbsolute("C:my-saves",1));
  CHECK(!LagoonHostPath_IsAbsolute("C:",1));
  CHECK(LagoonHostPath_IsAbsolute("C:/my-saves",1));
  CHECK(LagoonHostPath_IsAbsolute("C:\\my-saves",1));
  CHECK(LagoonHostPath_IsAbsolute("\\\\server\\share\\my-saves",1));
  CHECK(LagoonHostPath_IsSeparator('\\',1));
  CHECK(LagoonHostPath_IsSeparator('/',1));
  CHECK(!LagoonHostPath_IsSeparator(':',1));
  CHECK(!LagoonHostPath_IsAbsolute(NULL,0));
  CHECK(!LagoonHostPath_IsAbsolute("",0));
  CHECK(!LagoonHostPath_IsAbsolute("C",1));
  puts("PASS: POSIX and Windows save path grammars remain distinct.");
  return 0;
}
