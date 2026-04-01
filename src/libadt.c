#include "libadt.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct dynamic_array_header {
    size_t count;
    size_t capacity;
};

#define DYN_ARR_APPEND(arr, x)                                                                     \
    do {                                                                                           \
        if ((arr) == NULL) {                                                                       \
            struct dynamic_array_header *header =                                                  \
                malloc(sizeof(*arr) * 32 + sizeof(struct dynamic_array_header));                   \
            assert(header != NULL && "Failed to alloc dynamic array");                             \
            header->count = 0;                                                                     \
            header->capacity = 32;                                                                 \
            arr = (void *)(header + 1);                                                            \
        }                                                                                          \
        struct dynamic_array_header *header = (struct dynamic_array_header *)(arr) - 1;            \
        if (header->count >= header->capacity) {                                                   \
            header->capacity *= 2;                                                                 \
            header = realloc(header, sizeof(*arr) * header->capacity +                             \
                                         sizeof(struct dynamic_array_header));                     \
            assert(header != NULL && "Failed to realloc dynamic array");                           \
            arr = (void *)(header + 1);                                                            \
        }                                                                                          \
        (arr)[header->count++] = (x);                                                              \
    } while (0)

#define DYN_ARR_LEN(arr) ((arr) == NULL ? 0 : ((struct dynamic_array_header *)(arr) - 1)->count)
#define DYN_ARR_FREE(arr) free((struct dynamic_array_header *)(arr) - 1);

#define DYN_ARR_FOREACH(Type, iter, arr)                                                           \
    for (Type *iter = (arr); iter < (arr) + DYN_ARR_LEN((arr)); iter++)

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define PLACEHOLDER_BIT (1 << 31)

struct adt_stream {
    void *cur;
    void *end;
};

static bool adt_read(struct adt_stream *reader, void *out, size_t len)
{
    if (reader->cur + len > reader->end) {
        return false;
    }

    memcpy(out, reader->cur, len);
    reader->cur += len;
    return true;
}

static bool adt_write(struct adt_stream *writer, void *data, size_t len)
{
    if (writer->cur + len > writer->end) {
        return false;
    }

    memcpy(writer->cur, data, len);
    writer->cur += len;
    return true;
}

static bool adt_write_uint32(struct adt_stream *writer, uint32_t value)
{
    return adt_write(writer, &value, sizeof(uint32_t));
}

struct adt_node *adt_alloc()
{
    return calloc(1, sizeof(struct adt_node));
}

void adt_free(struct adt_node *node)
{
    if (node->props) {
        DYN_ARR_FREE(node->props);
    }

    if (node->childrens) {
        DYN_ARR_FOREACH(struct adt_node *, child, node->childrens)
        {
            adt_free(*child);
        }
        DYN_ARR_FREE(node->childrens);
    }

    free(node);
}

struct adt_node *adt_deserialize_node(struct adt_stream *reader)
{
    uint32_t prop_count;
    uint32_t children_count;

    if (!adt_read(reader, &prop_count, sizeof(uint32_t))) {
        return NULL;
    }

    if (!adt_read(reader, &children_count, sizeof(uint32_t))) {
        return NULL;
    }

    struct adt_node *node = adt_alloc();
    if (!node) {
        return NULL;
    }

    for (uint32_t i = 0; i < prop_count; i++) {
        struct adt_prop prop = {0};

        if (!adt_read(reader, prop.name, ADT_NAME_LEN)) {
            adt_free(node);
            return NULL;
        }

        prop.name[ADT_NAME_LEN - 1] = '\0';

        if (!adt_read(reader, &prop.len, sizeof(uint32_t))) {
            adt_free(node);
            return NULL;
        }

        if (prop.len & PLACEHOLDER_BIT) {
            prop.len &= ~PLACEHOLDER_BIT;
            prop.placeholder = true;
        }

        if (prop.len) {
            prop.data = malloc(prop.len);
            if (!prop.data) {
                adt_free(node);
                return NULL;
            }

            if (!adt_read(reader, prop.data, prop.len)) {
                adt_free(node);
                return NULL;
            }

            reader->cur = (void *)ALIGN_UP((uintptr_t)reader->cur, 4);
        }

        DYN_ARR_APPEND(node->props, prop);
    }

    for (uint32_t i = 0; i < children_count; i++) {
        struct adt_node *child = adt_deserialize_node(reader);
        if (!child) {
            adt_free(node);
            return NULL;
        }

        DYN_ARR_APPEND(node->childrens, child);
    }

    return node;
}

struct adt_node *adt_deserialize(void *adt, size_t size)
{
    struct adt_stream reader = {
        .cur = adt,
        .end = adt + size,
    };

    return adt_deserialize_node(&reader);
}

bool adt_serialize_node(struct adt_node *node, struct adt_stream *writer)
{
    if (!adt_write_uint32(writer, DYN_ARR_LEN(node->props))) {
        return false;
    }
    if (!adt_write_uint32(writer, DYN_ARR_LEN(node->childrens))) {
        return false;
    }

    DYN_ARR_FOREACH(struct adt_prop, prop, node->props)
    {
        if (!adt_write(writer, prop->name, ADT_NAME_LEN)) {
            return false;
        }

        uint32_t len = prop->len;
        if (prop->placeholder) {
            len |= PLACEHOLDER_BIT;
        }

        if (!adt_write_uint32(writer, len)) {
            return false;
        }

        if (prop->len) {
            if (!adt_write(writer, prop->data, prop->len)) {
                return false;
            }

            writer->cur = (void *)ALIGN_UP((uintptr_t)writer->cur, 4);
        }
    }

    DYN_ARR_FOREACH(struct adt_node *, child, node->childrens)
    {
        if (!adt_serialize_node(*child, writer)) {
            return false;
        }
    }

    return true;
}

bool adt_serialize(struct adt_node *node, void *buffer, size_t size)
{
    struct adt_stream writer = {
        .cur = buffer,
        .end = buffer + size,
    };

    return adt_serialize_node(node, &writer);
}

size_t adt_serialize_len(struct adt_node *node)
{
    size_t len = sizeof(uint32_t) * 2;

    DYN_ARR_FOREACH(struct adt_prop, prop, node->props)
    {
        len += ADT_NAME_LEN;
        len += sizeof(uint32_t);
        len += ALIGN_UP(prop->len, 4);
    }

    DYN_ARR_FOREACH(struct adt_node *, child, node->childrens)
    {
        len += adt_serialize_len(*child);
    }

    return len;
}

struct adt_node *adt_get_node(struct adt_node *node, const char *path)
{
    struct adt_prop *prop;
    const char *token;
    char *next;
    bool found;

    next = strdup(path);

    while (node != NULL && (token = strsep(&next, "/"))) {
        if (*token == '\0') {
            continue;
        }

        found = false;

        DYN_ARR_FOREACH(struct adt_node *, child, node->childrens)
        {
            prop = adt_get_prop(*child, "name");

            if (prop == NULL) {
                continue;
            }

            if (strcmp((const char *)prop->data, token) == 0) {
                node = *child;
                found = true;
                break;
            }
        }

        if (!found) {
            return NULL;
        }
    }

    return node;
}

void adt_child_add(struct adt_node *node, struct adt_node *child)
{
    if (child == NULL) {
        child = adt_alloc();
    }

    DYN_ARR_APPEND(node->childrens, child);
}

size_t adt_get_prop_count(struct adt_node *node)
{
    return DYN_ARR_LEN(node->props);
}

size_t adt_get_children_count(struct adt_node *node)
{
    return DYN_ARR_LEN(node->childrens);
}

struct adt_prop *adt_get_prop(struct adt_node *node, const char *name)
{
    DYN_ARR_FOREACH(struct adt_prop, prop, node->props)
    {
        if (strcmp(prop->name, name) == 0) {
            return prop;
        }
    }

    return NULL;
}

uint8_t adt_get_prop_uint8(struct adt_node *node, const char *name)
{
    struct adt_prop *prop = adt_get_prop(node, name);
    assert(prop != NULL && "Prop is missing");
    assert(prop->len == sizeof(uint8_t) && "Len mismatch");
    return *(uint8_t *)prop->data;
}

uint16_t adt_get_prop_uint16(struct adt_node *node, const char *name)
{
    struct adt_prop *prop = adt_get_prop(node, name);
    assert(prop != NULL && "Prop is missing");
    assert(prop->len == sizeof(uint16_t) && "Len mismatch");
    return *(uint16_t *)prop->data;
}

uint32_t adt_get_prop_uint32(struct adt_node *node, const char *name)
{
    struct adt_prop *prop = adt_get_prop(node, name);
    assert(prop != NULL && "Prop is missing");
    assert(prop->len == sizeof(uint32_t) && "Len mismatch");
    return *(uint32_t *)prop->data;
}

uint64_t adt_get_prop_uint64(struct adt_node *node, const char *name)
{
    struct adt_prop *prop = adt_get_prop(node, name);
    assert(prop != NULL && "Prop is missing");
    assert(prop->len == sizeof(uint64_t) && "Len mismatch");
    return *(uint64_t *)prop->data;
}

const char *adt_get_prop_string(struct adt_node *node, const char *name)
{
    struct adt_prop *prop = adt_get_prop(node, name);
    assert(prop != NULL && "Prop is missing");
    return prop->data;
}

struct adt_prop *adt_set_prop(struct adt_node *node, const char *name, const void *data,
                              uint32_t len)
{
    if (node == NULL || name == NULL)
        return NULL;

    struct adt_prop *prop = adt_get_prop(node, name);
    if (prop) {
        free(prop->data);
    } else {
        struct adt_prop _prop = {0};
        strcpy(_prop.name, name);
        DYN_ARR_APPEND(node->props, _prop);

        prop = adt_get_prop(node, name);
    }

    prop->data = calloc(len, sizeof(uint8_t));
    prop->len = len;
    if (data) {
        memcpy(prop->data, data, len);
    }

    return prop;
}

struct adt_prop *adt_set_prop_null(struct adt_node *node, const char *name)
{
    return adt_set_prop(node, name, NULL, 0);
}

struct adt_prop *adt_set_prop_uint8(struct adt_node *node, const char *name, uint8_t value)
{
    return adt_set_prop(node, name, &value, sizeof(uint8_t));
}

struct adt_prop *adt_set_prop_uint16(struct adt_node *node, const char *name, uint16_t value)
{
    return adt_set_prop(node, name, &value, sizeof(uint16_t));
}

struct adt_prop *adt_set_prop_uint32(struct adt_node *node, const char *name, uint32_t value)
{
    return adt_set_prop(node, name, &value, sizeof(uint32_t));
}

struct adt_prop *adt_set_prop_uint64(struct adt_node *node, const char *name, uint64_t value)
{
    return adt_set_prop(node, name, &value, sizeof(uint64_t));
}

struct adt_prop *adt_set_prop_string(struct adt_node *node, const char *name, const char *value)
{
    return adt_set_prop(node, name, value, strlen(value) + 1);
}

struct adt_prop *adt_set_prop_stringn(struct adt_node *node, const char *name, const char *value,
                                      uint32_t len)
{
    return adt_set_prop(node, name, value, len);
}
