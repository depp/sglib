#ifndef WIN_DEFS_H
#define WIN_DEFS_H
#define UNICODE
#include <Windows.h>
#ifdef __cplusplus
extern "C" {
#endif

struct exe_options {
    LPWSTR alt_data_dir;
};

void path_init(exe_options *opts);

#ifdef __cplusplus
}
#endif
#endif
