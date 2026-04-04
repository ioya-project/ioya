#include <stdbool.h>
#include <symbols.h>

int SecureDTFindEntry(const char *propName, const char *propValue, DTEntry *entryH)
{
    if (*symbols.dt.initialized == false) {
        return kError;
    }

    *symbols.dt.starting_p = *symbols.dt.root_node;
    return symbols.dt.find_entry(propName, propValue, entryH);
}