#ifndef LIBADT_H_
#define LIBADT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ADT_NAME_LEN 32

struct adt_node;

struct adt_prop {
    char name[ADT_NAME_LEN];
    uint32_t len;
    void *data;
    bool placeholder;
};

struct adt_node {
    struct adt_prop *props;
    struct adt_node **childrens;
};

struct adt_node *adt_alloc();
void adt_free(struct adt_node *node);

struct adt_node *adt_deserialize(void *adt, size_t size);
bool adt_serialize(struct adt_node *node, void *buffer, size_t len);
size_t adt_serialize_len(struct adt_node *node);

struct adt_node *adt_get_node(struct adt_node *node, const char *path);
void adt_child_add(struct adt_node *node, struct adt_node *child);

size_t adt_get_prop_count(struct adt_node *node);
size_t adt_get_children_count(struct adt_node *node);

struct adt_prop *adt_get_prop(struct adt_node *node, const char *name);
uint8_t adt_get_prop_uint8(struct adt_node *node, const char *name);
uint16_t adt_get_prop_uint16(struct adt_node *node, const char *name);
uint32_t adt_get_prop_uint32(struct adt_node *node, const char *name);
uint64_t adt_get_prop_uint64(struct adt_node *node, const char *name);
const char *adt_get_prop_string(struct adt_node *node, const char *name);

struct adt_prop *adt_set_prop(struct adt_node *node, const char *name, const void *data,
                              uint32_t len);
struct adt_prop *adt_set_prop_null(struct adt_node *node, const char *name);
struct adt_prop *adt_set_prop_uint8(struct adt_node *node, const char *name, uint8_t value);
struct adt_prop *adt_set_prop_uint16(struct adt_node *node, const char *name, uint16_t value);
struct adt_prop *adt_set_prop_uint32(struct adt_node *node, const char *name, uint32_t value);
struct adt_prop *adt_set_prop_uint64(struct adt_node *node, const char *name, uint64_t value);
struct adt_prop *adt_set_prop_string(struct adt_node *node, const char *name, const char *value);
struct adt_prop *adt_set_prop_stringn(struct adt_node *node, const char *name, const char *value,
                                      uint32_t len);

#endif