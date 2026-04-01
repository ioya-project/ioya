#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include <config.h>
#include <fat.h>

void config_parser_parse(char *buf, uint32_t len);
void config_parser_validate();

#endif