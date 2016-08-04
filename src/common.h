#ifndef COMMON_H
#define COMMON_H

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long i64;
typedef int i32;
typedef short i16;
typedef char i8;
typedef float f32;
typedef double f64;

// Allocates a string, must be freed by user
char *file_to_string(char *filename) {
	FILE *file = fopen(filename, "r");

	fseek(file, 0, SEEK_END);
	u64 length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *file_string = malloc(length + 1);
	fread(file_string, 1, length, file);
	file_string[length] = 0;

	fclose(file);
	return file_string;
}

#endif
