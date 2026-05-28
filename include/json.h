#pragma once

#include <SDL3/SDL_stdinc.h>

#include <stddef.h>

typedef struct
{
	size_t pos;
	unsigned int next;
	int super;
} json_parser_t;

typedef enum
{
	JSON_UNDEFINED = 0,
	JSON_OBJECT    = 1,
	JSON_ARRAY     = 2,
	JSON_STRING    = 3,
	JSON_PRIMITIVE = 4,
} json_type_t;

typedef enum
{
	JSON_ERROR_OOM        = -1,
	JSON_ERROR_INVALID    = -2,
	JSON_ERROR_INCOMPLETE = -3
} json_error_t;

typedef struct
{
	json_type_t type;
	ptrdiff_t start;
	ptrdiff_t end;
	int size;
	int parent;
} json_token_t;

void json_init(json_parser_t *parser);

int json_parse(json_parser_t *parser, const char *json, size_t json_len,
	json_token_t *tokens, size_t token_count);
