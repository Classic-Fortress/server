// version.h

#ifdef _WIN32
#define QW_PLATFORM	"Win32"
#define QW_PLATFORM_SHORT "(w)"
#endif

#ifdef __FreeBSD__
#define QW_PLATFORM	"FreeBSD"
#define QW_PLATFORM_SHORT "(f)"
#endif

#ifdef __linux__
#define QW_PLATFORM	"Linux"
#define QW_PLATFORM_SHORT "(l)"
#endif

#ifdef sun
#define QW_PLATFORM	"Sun"
#define QW_PLATFORM_SHORT "(s)"
#endif

#define	QW_VERSION		2.40
#define QWE_VERSION	"0.1724"
#define QWE_VERNUM 0.1724
#define LINUX_VERSION 0.98
#define PROJECT_NAME	"QWExtended"
#define SERVER_NAME	"MVDSV"
#define FULL_VERSION SERVER_NAME " " QWE_VERSION " " QW_PLATFORM_SHORT ", build %d\nBuild date: " __DATE__ ", " __TIME__ "\0\0\0\0\0\0"

#define RELEASE_VERSION

extern char full_version[];

int build_number (void);
void Version_f (void);
