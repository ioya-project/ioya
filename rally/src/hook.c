#include <hook.h>
#include <mem.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void install_hook(const char *name, void *target, void *hook_fn, struct hook *h)
{
    h->target = target;
    h->hook_fn = hook_fn;
    h->original = PA_TO_VA((void *)h->saved);

    memcpy(h->saved, VA_TO_PA(target), TRAMPOLINE_SIZE);

    TRAMPOLINE_JUMP(h->saved + TRAMPOLINE_SIZE, target + TRAMPOLINE_SIZE);
    TRAMPOLINE_JUMP(VA_TO_PA(target), PA_TO_VA(hook_fn));

    printf("Installed hook '%s' from 0x%p to 0x%p\n", name, target, PA_TO_VA(hook_fn));
}