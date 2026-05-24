/*==========================================================================
  xbox_fixes.h - Compiler/platform fixes for Xbox XDK port.
  Included via ForcedInclude in the vcxproj for all C/C++ files.
==========================================================================*/
#ifndef XBOX_FIXES_H
#define XBOX_FIXES_H

/* OutputDebugStringA - in xbdm.lib (debug only).
   Map to DbgPrint which is always available in xboxkrnl.lib. */
#ifndef OutputDebugStringA
#define OutputDebugStringA(s) DbgPrint("%s", (s))
#endif

#endif /* XBOX_FIXES_H */
