#ifndef PTBR_MODDING_H
#define PTBR_MODDING_H
#define RECOMP_PATCH __attribute__((section(".recomp_patch")))
#define RECOMP_FORCE_PATCH __attribute__((section(".recomp_force_patch")))
#define RECOMP_CALLBACK(mod, event) __attribute__((section(".recomp_callback." mod ":" #event)))
#define RECOMP_HOOK(func) __attribute__((section(".recomp_hook." func)))
#define RECOMP_HOOK_RETURN(func) __attribute__((section(".recomp_hook_return." func)))
#endif
