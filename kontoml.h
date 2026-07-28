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
	konTomlBool
} kon_tomlType_t;

typedef struct kon_tomlEntry {
	char *section;
	char *key;
	kon_tomlType_t type;
	char *str_value;
	long long int_value;
	double float_value;
	bool bool_value;
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

/*** implementation ***/

#ifdef KONTOML_IMPLEMENTATION

#include <string.h>
#include <ctype.h>
#include <stdio.h>

static char *kon_tomlStrdup(const char *s) {
	size_t len = strlen(s) + 1;
	char *out = malloc(len);
	if (!out) return NULL;
	memcpy(out, s, len);
	return out;
}

static char *kon_tomlStrndup(const char *s, size_t n) {
	char *out = malloc(n + 1);
	if (!out) return NULL;
	memcpy(out, s, n);
	out[n] = '\0';
	return out;
}

#endif /* end of KONTOML_IMPLEMENTATION */

#endif
