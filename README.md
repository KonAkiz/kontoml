
# KONTOML
## KonAki's TOML Parser

![License](https://img.shields.io/badge/license-MIT-blue)
![Language](https://img.shields.io/badge/language-C99-orange)

## About

This project was made as a single header library for parsing toml files for my projects. It is a light implementation that can read a basic toml file.

It can only support basic toml files. You can check out my [example.toml](example.toml) to see the toml file version it's meant to handle.

## Example

The Example I made uses a [Makefile](Makefile) to be built. This particular example was built with my windowing library headers included so that you can test around with the toml settings to edit where the window goes and resizes.

You can see some example files in the [examples directory](examples).

In fact, here's a basic example, right here:

```c
/* define KONTOML_IMPLEMENTATION in one file, the one that you wish to store the source code into */
#define KONTOML_IMPLEMENTATION
#include "kontoml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define FILE_PATH "example.toml"

typedef struct file_data {
	bool random_cat;
	char title[64];
} file_data_t;

/*** loading file data and storing it ***/

static bool loadFileData(const char *filePath, file_data_t *data) {
	kon_toml_t *toml = kon_parseTomlFile(filePath);
	if (!toml) {
		fprintf(stderr, "kon_parseTomlFile: error parsing %s\n", filePath);
		return false;
	}

	data->random_cat = kon_tomlGetBool(toml, "", "random_cat", false);

	strcpy(data->title, kon_tomlGetString(toml, "window", "title", "default"));

	kon_freeToml(toml);

	return true;
}

int main(void) {
	file_data_t data; /* <- where we store the data */

	if (!loadFileData(FILE_PATH, &data)) return EXIT_FAILURE;

	/*** using the data ***/

	printf("random_cat %s\n", (data.random_cat) ? "true" : "false");
	printf("title: %s\n", data.title);

	return EXIT_SUCCESS;
}
```

This example was made so that you can copy and use the [example.toml file](example.toml)

## Build

You don't need any flags if you're going to compile it manually.
```sh
cc -std=c99 main.c -o output -I.
```

Just include the header and in the .c files and -I the directory that hold it during compile time.

### Build systems

I haven't built this project with cmake since I primarily use GNU Make when making projects.

Here's how to use my GNU Make setup to build the example. You can read it as well if you want to learn how to properly build my project.

<details>
<summary>Makefile build</summary>

This was primarily tested with GNU Make. But, I am trying to make my build scripts as POSIX compliant as possible.

Just enter the project root and type this to build it.
```sh
make
```

If you want to cleanup the build, run this:
```sh
make clean
```

If you wish to test it out, run this:
```sh
make test
```

</details>

## LICENSE

[MIT License](LICENSE)
