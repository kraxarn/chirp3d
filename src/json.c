#include "json.h"

#define CGLTF_MALLOC(size) SDL_malloc(size)
#define CGLTF_FREE(ptr)    SDL_free(ptr)
#define CGLTF_ATOI(str)    SDL_atoi(str)
#define CGLTF_ATOF(str)    SDL_atof(str)
#define CGLTF_ATOLL(str)   SDL_strtoll(str,nullptr,10)

#ifndef NDEBUG
#define CGLTF_VALIDATE_ENABLE_ASSERTS 1
#endif

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

static_assert(sizeof(json_parser_t) == sizeof(jsmn_parser));
static_assert(sizeof(json_token_t) == sizeof(jsmntok_t));
static_assert(sizeof(json_type_t) == sizeof(jsmntype_t));

void json_init(json_parser_t *parser)
{
	jsmn_init((jsmn_parser*) parser);
}

int json_parse(json_parser_t *parser, const char *json, const size_t json_len,
	json_token_t *tokens, const size_t token_count)
{
	return jsmn_parse((jsmn_parser*) parser, json, json_len,
		(jsmntok_t*) tokens, token_count);
}
