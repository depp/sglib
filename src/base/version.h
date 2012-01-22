#ifndef VERSION_H
#define VERSION_H
#ifdef __cplusplus
extern "C" {
#endif

extern const char SG_VERSION[];

void
sg_version_lib(struct sg_logger *lp, const char *libname,
               const char *compileversion, const char *runversion);

void
sg_version_print(void);

#ifdef __cplusplus
}
#endif
#endif
