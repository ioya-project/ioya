#include <pexpert/device_tree.h>

extern char const *startingP;
extern void *DTRootNode;
extern int DTInitialized;

static int (*find_entry)(const char *propName, const char *propValue,
                         DTEntry *entryH) = (void *)0xfffffff00840c9c8;

int SecureDTFindEntry(const char *propName, const char *propValue, DTEntry *entryH)
{
    if (!DTInitialized) {
        return kError;
    }

    startingP = (char const *)DTRootNode;
    return find_entry(propName, propValue, entryH);
}