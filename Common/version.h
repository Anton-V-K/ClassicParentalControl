#ifndef CPC_version_h_
#define CPC_version_h_

#ifdef _DEBUG
#  define VERSION_CONFIG " Debug"
#else
#  define VERSION_CONFIG
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_REV   0

#define VERSION_STRING "1.1.0 Alpha" VERSION_CONFIG

#endif
