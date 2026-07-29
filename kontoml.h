/* SPDX-License-Identifier: MIT */
/*
 * kontoml.h
 *
 * Copyright (c) 2026 KonAki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef KONTOML_H
#define KONTOML_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum kon_tomlType {
	konTomlString,
	konTomlInt,
	konTomlFloat,
	konTomlBool,
	konTomlArray
} kon_tomlType_t;

typedef struct kon_tomlEntry {
	char *section;
	char *key;
	kon_tomlType_t type;
	char *str_value;
	long long int_value;
	double float_value;
	bool bool_value;

	kon_tomlType_t array_type;
	int array_count;
	char **array_str;
	long long *array_int;
	double *array_float;
	bool *array_bool;
} kon_tomlEntry_t;

typedef struct kon_toml {
	kon_tomlEntry_t *entries;
	int count;
	int capacity;
} kon_toml_t;

kon_toml_t *kon_parseToml(const char *text);
kon_toml_t *kon_parseTomlFile(const char *path);
void kon_freeToml(kon_toml_t *toml);

const char *kon_tomlGetString(kon_toml_t *toml, const char *section, const char *key, const char *def);
long long kon_tomlGetInt(kon_toml_t *toml, const char *section, const char *key, long long def);
double kon_tomlGetFloat(kon_toml_t *toml, const char *section, const char *key, double def);
bool kon_tomlGetBool(kon_toml_t *toml, const char *section, const char *key, bool def);

const char *const *kon_tomlGetStringArray(kon_toml_t *toml, const char *section, const char *key, int *out_count);
const long long *kon_tomlGetIntArray(kon_toml_t *toml, const char *section, const char *key, int *out_count);
const double *kon_tomlGetFloatArray(kon_toml_t *toml, const char *section, const char *key, int *out_count);
const bool *kon_tomlGetBoolArray(kon_toml_t *toml, const char *section, const char *key, int *out_count);

/*** implementation ***/

#ifdef KONTOML_IMPLEMENTATION

#include <string.h>
#include <ctype.h>
#include <stdio.h>

static char *kon__tomlStrdup(const char *s) {
	size_t len = strlen(s) + 1;
	char *out = malloc(len);
	if (!out) return NULL;
	memcpy(out, s, len);
	return out;
}

static char *kon__tomlStrndup(const char *s, size_t n) {
	char *out = malloc(n + 1);
	if (!out) return NULL;
	memcpy(out, s, n);
	out[n] = '\0';
	return out;
}

static char *kon__tomlTrim(char *s) {
	char *end;

	while (*s == ' ' || *s == '\t') s++;
	if (*s == '\0') return s;

	end = s + strlen(s) - 1;
	while (end > s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
		*end = '\0';
		end--;
	}

	return s;
}

static kon_tomlEntry_t *kon__tomlPushEntry(kon_toml_t *toml) {
	if (toml->count == toml->capacity) {
		int newCap = toml->capacity ? toml->capacity * 2 : 16;
		kon_tomlEntry_t *tmp = realloc(toml->entries, (size_t)newCap * sizeof(kon_tomlEntry_t));
		if (!tmp) return NULL;

		toml->entries = tmp;
		toml->capacity = newCap;
	}

	kon_tomlEntry_t *entry = &toml->entries[toml->count];
	memset(entry, 0, sizeof(kon_tomlEntry_t));
	toml->count++;

	return entry;
}

static int kon__tomlParseString(const char **p, char **out) {
	const char *s = *p;
	char quote = *s;
	int literal = (quote == '\'');
	s++;

	char *buf = malloc(strlen(s) + 1);
	if (!buf) return -1;

	size_t bi = 0;

	while (*s && *s != quote) {
		if (!literal && *s == '\\') {
			s++;
			switch (*s) {
			case 'n':
				buf[bi++] = '\n';
				break;
			case 't':
				buf[bi++] = '\t';
				break;
			case 'r':
				buf[bi++] = '\r';
				break;
			case '"':
				buf[bi++] = '"';
				break;
			case '\\':
				buf[bi++] = '\\';
				break;
			case '\0':
				free(buf);
				return -1;
			default:
				buf[bi++] = *s;
				break;
			}
			s++;
		} else {
			buf[bi++] = *s;
			s++;
		}
	}

	if (*s != quote) {
		free(buf);
		return -1;
	}
	s++;

	buf[bi] = '\0';
	*out = buf;
	*p = s;

	return 0;
}

static char *kon__tomlStripUnderscores(const char *s) {
	char *out = malloc(strlen(s) + 1);
	if (!out) return NULL;

	size_t oi = 0;
	for (size_t i = 0; s[i]; i++) {
		if (s[i] != '_') out[oi++] = s[i];
	}
	out[oi] = '\0';

	return out;
}

static char **kon__tomlSplitArrayElements(const char *content, int *outCount) {
	int cap = 8;
	int count = 0;
	char **elems = malloc((size_t)cap * sizeof(char *));
	if (!elems) return NULL;

	const char *start = content;
	const char *p = content;
	char inQuote = 0;

	while (*p) {
		if (inQuote) {
			if (inQuote == '"' && *p == '\\' && *(p + 1) != '\0') {
				p += 2;
				continue;
			}
			if (*p == inQuote) inQuote = 0;
			p++;
			continue;
		}

		if (*p == '"' || *p == '\'') {
			inQuote = *p;
			p++;
			continue;
		}

		if (*p == ',') {
			if (count == cap) {
				cap *= 2;
				char **tmp = realloc(elems, (size_t)cap * sizeof(char *));
				if (!tmp) { free(elems); return NULL; }
				elems = tmp;
			}
			elems[count++] = kon__tomlStrndup(start, (size_t)(p - start));
			p++;
			start = p;
			continue;
		}

		p++;
	}

	{
		char *raw = kon__tomlStrndup(start, (size_t)(p - start));
		char *checkTrim = raw ? kon__tomlTrim(raw) : NULL;

		if (raw && *checkTrim != '\0') {
			if (count == cap) {
				cap += 1;
				char **tmp = realloc(elems, (size_t)cap * sizeof(char *));
				if (!tmp) { free(raw); free(elems); return NULL; }
				elems = tmp;
			}
			elems[count++] = raw;
		} else {
			free(raw);
		}
	}

	*outCount = count;
	return elems;
}

static int kon__tomlParseArrayValue(kon_tomlEntry_t *entry, const char *valueStr) {
	const char *open  = strchr(valueStr, '[');
	const char *close = strchr(valueStr, ']');
	if (!open || !close || close <= open) return -1;

	char *content = kon__tomlStrndup(open + 1, (size_t)(close - open - 1));
	if (!content) return -1;

	int rawCount = 0;
	char **rawElems = kon__tomlSplitArrayElements(content, &rawCount);
	free(content);
	if (!rawElems) return -1;

	if (rawCount == 0) {
		free(rawElems);
		entry->type = konTomlArray;
		entry->array_type = konTomlInt;
		entry->array_count = 0;
		return 0;
	}

	kon_tomlType_t elemType = konTomlInt;
	char **strArr = NULL;
	long long *intArr = NULL;
	double *floatArr = NULL;
	bool *boolArr = NULL;
	int ok = 1;

	for (int i = 0; i < rawCount && ok; i++) {
		char *trimmed = kon__tomlTrim(rawElems[i]);

		kon_tomlType_t thisType;
		char *decodedStr = NULL;
		long long thisInt = 0;
		double thisFloat = 0.0;
		bool thisBool = false;

		if (*trimmed == '"' || *trimmed == '\'') {
			const char *vp = trimmed;
			thisType = konTomlString;
			if (kon__tomlParseString(&vp, &decodedStr) != 0) { ok = 0; break; }
		} else if (strcmp(trimmed, "true") == 0) {
			thisType = konTomlBool;
			thisBool = true;
		} else if (strcmp(trimmed, "false") == 0) {
			thisType = konTomlBool;
			thisBool = false;
		} else {
			char *clean = kon__tomlStripUnderscores(trimmed);
			if (!clean) { ok = 0; break; }

			int isFloat = (strchr(clean, '.') || strchr(clean, 'e') || strchr(clean, 'E'));
			if (isFloat) {
				thisType = konTomlFloat;
				thisFloat = strtod(clean, NULL);
			} else {
				thisType = konTomlInt;
				thisInt = strtoll(clean, NULL, 10);
			}
			free(clean);
		}

		if (i == 0) {
			elemType = thisType;
			switch (elemType) {
			case konTomlString:
				strArr = malloc((size_t)rawCount * sizeof(char *));
				break;
			case konTomlInt:
				intArr = malloc((size_t)rawCount * sizeof(long long));
				break;
			case konTomlFloat:
				floatArr = malloc((size_t)rawCount * sizeof(double));
				break;
			case konTomlBool:
				boolArr = malloc((size_t)rawCount * sizeof(bool));
				break;
			default:
				break;
			}
		} else if (thisType != elemType) {
			free(decodedStr);
			ok = 0;
			break;
		}

		switch (elemType) {
		case konTomlString:
			strArr[i] = decodedStr;
			break;
		case konTomlInt:
			intArr[i] = thisInt;
			break;
		case konTomlFloat:
			floatArr[i] = thisFloat;
			break;
		case konTomlBool:
			boolArr[i] = thisBool;
			break;
		}
	}

	for (int i = 0; i < rawCount; i++) free(rawElems[i]);
	free(rawElems);

	if (!ok) {
		if (strArr) {
			for (int i = 0; i < rawCount; i++)
				free(strArr[i]);

			free(strArr);
		}
		free(intArr);
		free(floatArr);
		free(boolArr);
		return -1;
	}

	entry->type = konTomlArray;
	entry->array_type = elemType;
	entry->array_count = rawCount;
	entry->array_str = strArr;
	entry->array_int = intArr;
	entry->array_float = floatArr;
	entry->array_bool = boolArr;

	return 0;
}

static int kon__tomlParseLine(kon_toml_t *toml, char *line, char **currentSection) {
	char *trimmed = kon__tomlTrim(line);

	if (*trimmed == '\0' || *trimmed == '#') return 0;

	if (*trimmed == '[') {
		char *close = strchr(trimmed, ']');
		if (!close) return -1;

		free(*currentSection);
		*currentSection = kon__tomlStrndup(trimmed + 1, (size_t)(close - trimmed - 1));

		return 0;
	}

	char *eq = strchr(trimmed, '=');
	if (!eq) return -1;

	*eq = '\0';
	char *key = kon__tomlTrim(trimmed);
	char *valueStr = kon__tomlTrim(eq + 1);

	if (*valueStr != '"' && *valueStr != '\'') {
		char *hash = strchr(valueStr, '#');
		if (hash) {
			*hash = '\0';
			valueStr = kon__tomlTrim(valueStr);
		}
	}

	if (*key == '\0' || *valueStr == '\0') return -1;

	kon_tomlEntry_t *entry = kon__tomlPushEntry(toml);
	if (!entry) return -1;

	entry->section = *currentSection ? kon__tomlStrdup(*currentSection) : kon__tomlStrdup("");
	entry->key = kon__tomlStrdup(key);

	if (*valueStr == '"' || *valueStr == '\'') {
		const char *vp = valueStr;
		char *decoded = NULL;

		if (kon__tomlParseString(&vp, &decoded) != 0) return -1;

		entry->type = konTomlString;
		entry->str_value = decoded;
	} else if (strcmp(valueStr, "true") == 0) {
		entry->type = konTomlBool;
		entry->bool_value = true;
	} else if (strcmp(valueStr, "false") == 0) {
		entry->type = konTomlBool;
		entry->bool_value = false;
	} else {
		char *clean = kon__tomlStripUnderscores(valueStr);
		if (!clean) return -1;

		int isFloat = (strchr(clean, '.') || strchr(clean, 'e') || strchr(clean, 'E'));

		if (isFloat) {
			entry->type = konTomlFloat;
			entry->float_value = strtod(clean, NULL);
		} else {
			entry->type = konTomlInt;
			entry->int_value = strtoll(clean, NULL, 10);
		}

		free(clean);
	}

	return 0;
}

kon_toml_t *kon_parseToml(const char *text) {
	if (!text) return NULL;

	kon_toml_t *toml = malloc(sizeof(kon_toml_t));
	if (!toml) return NULL;

	toml->entries = NULL;
	toml->count = 0;
	toml->capacity = 0;

	char *currentSection = NULL;

	const char *lineStart = text;
	while (*lineStart) {
		const char *lineEnd = strchr(lineStart, '\n');
		size_t len = lineEnd ? (size_t)(lineEnd - lineStart) : strlen(lineStart);

		char *line = kon__tomlStrndup(lineStart, len);
		if (line) {
			kon__tomlParseLine(toml, line, &currentSection);
			free(line);
		}

		if (!lineEnd) break;
		lineStart = lineEnd + 1;
	}

	free(currentSection);

	return toml;
}

kon_toml_t *kon_parseTomlFile(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size < 0) {
		fclose(f);
		return NULL;
	}

	char *buf = malloc((size_t)size + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	size_t read = fread(buf, 1, (size_t)size, f);
	buf[read] = '\0';
	fclose(f);

	kon_toml_t *toml = kon_parseToml(buf);
	free(buf);

	return toml;
}

void kon_freeToml(kon_toml_t *toml) {
	if (!toml) return;

	for (int i = 0; i < toml->count; i++) {
		free(toml->entries[i].section);
		free(toml->entries[i].key);
		if (toml->entries[i].type == konTomlString) {
			free(toml->entries[i].str_value);
		}
	}

	free(toml->entries);
	free(toml);
}

static kon_tomlEntry_t *kon__tomlFind(kon_toml_t *toml, const char *section, const char *key) {
	if (!toml) return NULL;

	const char *sec = section ? section : "";

	for (int i = 0; i < toml->count; i++) {
		kon_tomlEntry_t *e = &toml->entries[i];
		if (strcmp(e->section, sec) == 0 && strcmp(e->key, key) == 0) {
			return e;
		}
	}

	return NULL;
}

const char *kon_tomlGetString(kon_toml_t *toml, const char *section, const char *key, const char *def) {
	kon_tomlEntry_t *e = kon__tomlFind(toml, section, key);
	if (!e || e->type != konTomlString) return def;
	return e->str_value;
}

long long kon_tomlGetInt(kon_toml_t *toml, const char *section, const char *key, long long def) {
	kon_tomlEntry_t *e = kon__tomlFind(toml, section, key);
	if (!e) return def;

	if (e->type == konTomlInt) return e->int_value;
	if (e->type == konTomlFloat) return (long long)e->float_value;

	return def;
}

double kon_tomlGetFloat(kon_toml_t *toml, const char *section, const char *key, double def) {
	kon_tomlEntry_t *e = kon__tomlFind(toml, section, key);
	if (!e) return def;

	if (e->type == konTomlFloat) return e->float_value;
	if (e->type == konTomlInt) return (double)e->int_value;

	return def;
}

bool kon_tomlGetBool(kon_toml_t *toml, const char *section, const char *key, bool def) {
	kon_tomlEntry_t *e = kon__tomlFind(toml, section, key);
	if (!e || e->type != konTomlBool) return def;
	return e->bool_value;
}

#endif /* END of KONTOML_IMPLEMENTATION */

#endif
