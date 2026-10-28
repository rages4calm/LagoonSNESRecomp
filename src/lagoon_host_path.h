#ifndef LAGOON_HOST_PATH_H
#define LAGOON_HOST_PATH_H

/* Host paths are not guest paths. On POSIX, a backslash and a colon are legal
 * filename characters, not Windows separators/drive prefixes. Keep the policy
 * explicit so both platform grammars can be checked without a filesystem. */
#ifdef _WIN32
#define LAGOON_HOST_WINDOWS_PATHS 1
#else
#define LAGOON_HOST_WINDOWS_PATHS 0
#endif

static inline int LagoonHostPath_IsSeparator(char c, int windows) {
  return c == '/' || (windows && c == '\\');
}

static inline int LagoonHostPath_IsAbsolute(const char *path, int windows) {
  if (!path || !*path) return 0;
  if (path[0] == '/') return 1;
  if (!windows) return 0;
  if (path[0] == '\\') return 1;
  return ((path[0] >= 'A' && path[0] <= 'Z') ||
          (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':' &&
         LagoonHostPath_IsSeparator(path[2], 1);
}

#endif
