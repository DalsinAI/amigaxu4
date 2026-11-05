/* A Shell gives 4 KB of stack; xu4's C++ (the XML loaders, the maps and
 * the intro) wants far more. libnix's startup sets this much up.
 * The AmiCradle Team. GPL 2 or later, like xu4. */
unsigned long __stack = 512 * 1024;
static const char version[] __attribute__((used)) = "$VER: xu4 1.1svn Amiga (05.11.2026)";
