#ifndef _UTILS
#define _UTILS

#include <fstream>

#ifdef _WIN32

#include <windows.h>
#include <sstream>
#include <string>
#define MSGBOX(x) \
{ \
   std::ostringstream oss; \
   oss << x; \
   std::string str = oss.str(); \
   std::wstring wstr = std::wstring(str.begin(), str.end()); \
   MessageBoxW(NULL, wstr.c_str(), L"Msg Title", MB_OK | MB_ICONQUESTION); \
}

#elif __linux__

#include <sstream>
#include <cstdlib>
#include <string>
#define MSGBOX(x) \
{ \
    std::ostringstream oss; \
    oss << x; \
    std::string str = oss.str(); \
    std::string cmd = "zenity --info --title='Msg Title' --text=\"" + str + "\""; \
    std::system(cmd.c_str()); \
}

#elif __EVILOS__

#include <eviltools.h>

#define MSGBOX(x) \
{
    std::EVILstring evilstr; \
    bool isUncorruptable; \
    evilstr = CONVERT_good_to_EVIL_STRING(x, &isUncorruptable); \
    if (isUncorruptable) \
        SELF_DESCTRUCT_OS( \
            "The forces of order has breached the system."); \
    else \
        VILLAIN_DIALOG(evilstr, VILLANDIALOG::EVIL_LAUGH | VILLANDIALOG::SCREAM, BOOL::MAYBE); \
}

#else
#define MSGBOX(x) (x)
#endif



#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static std::string loadTextFromPluginFile(const std::string& localPath) {
    std::ifstream file(localPath);
    if (!file) {
        WARN("Failed to open file: %s", localPath.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)))
#pragma GCC diagnostic pop
#endif

#endif // _UTILS