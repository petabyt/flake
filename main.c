#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Holds a temporary buffer for string expanding
#define BUFFER_SIZE 10 * 1024
char *buffer = NULL;

char *nothing = "";

#define MAX_MACRO 1000
struct Macros {
	char *name;
	char *value;
	// file?
	// line?
	// type?
}macros[MAX_MACRO] = {
	{"IS_FLAKE", "0.1.0"}
};
int macroLen = 0;

#define MAX_TARGET 100
struct Targets {
	char *name;
	char *buf;
	int pos;
}targets[MAX_TARGET];
int targetLen = 0;

// Assumes all parameters are allocated
void addMacro(char name[], char value[]) {
	macros[macroLen].name = name;
	macros[macroLen].value = value;
	macroLen++;
}

// Also assumes parameters are allocated
void addTarget(char buf[], int pos, char name[]) {
	targets[targetLen].name = name;
	targets[targetLen].pos = pos;
	targets[targetLen].buf = buf;
	targetLen++;
}

char *getMacro(char name[]) {
	for (int i = 0; i < macroLen; i++) {
		if (!strcmp(macros[i].name, name)) {
			return macros[i].value;
		}
	}

	return nothing;
}

// All valid characters for a macro name
int validChar(char c) {
	return (c != ':' && c != '=' && c != '\0' && c != '\n');
}

// run a command, with '@', '-', flags
int runCommand(char command[]) {
	int silent = 0;
	int noerror = 0;
	while (1) {
		if (*command == '@') {
			silent = 1;
		} else if (*command == '-') {
			noerror = 1;
		} else {
			break;
		}

		command++;
	}

	if (!silent) {
		puts(command);
	}

	int ret = system(command);
	if (noerror) {
		return 0;
	}

	return ret;
}

// Use processed to tell whether all macros in string
// have been implemented. Can be NULL and won't do it
char *processString(char string[], int *processed) {
	// TODO: allocate bigger buffer?
	char *buf = malloc(1024);
	buf[0] = '\0';

	if (processed != NULL) {*processed = 1;}

	for (int c = 0; string[c] != '\0'; c++) {
		if (string[c] == '$') {
			if (processed != NULL) {*processed = 0;}
			c++;

			// Basic processing stack. processString will
			// be called recursively, for nested macros
			int stack = 1;
			if (string[c] == '(') {
				c++;
				int origin = c;

				// Skip space before stuff $( info asd)
				while (string[c] == ' ' || string[c] == '\t') {c++;}

				while (string[c] != '\n') {
					if (string[c] == '(') {
						stack++;
					} else if (string[c] == ')') {
						stack--;
					} else if (string[c] == '\0') {
						puts("Expected an ending )");
						exit(1);
					}

					if (stack == 0) {
						break;
					}

					c++;
				}

				// Allocate and store the value
				int len = c - origin;
				char *newString = malloc(len);
				memcpy(newString, string + origin, len);
				newString[len] = '\0';

				// TODO: calls to processString must be called
				// recursively

				// Process some basic functions
				int c2 = 0;
				if (!strncmp(newString, "info", 4)) {
					c2 += 4;
					while (newString[c2] == ' ' || newString[c2] == '\t') {c2++;}
					puts(processString(newString + c2, NULL));
				} else {
					// Process regular macro references
					// TODO: recursive macros $(((TEST)))
					strcat(buf, getMacro(newString));
				}
			}
		} else {
			// Add regular char to buf (convert it to string
			// in somewhat hacky way)
			strcat(buf, (char[]){string[c], '\0'});
		}
	}

	return buf;
}

int runTarget(char name[]) {
	// TODO: process %
	// TODO: process macros in target
	int i;
	for (i = 0; i < targetLen; i++) {
		if (!strcmp(targets[i].name, name)) {
			goto found;
			return 0;
		}
	}

	printf("No rule to make target %s\n", name);
	return 1;

	found:;

	// Skip to start of 
	char *buf = targets[i].buf;
	int c = targets[i].pos;
	while (buf[c] != '\n') {
		c++;
	}

	// Jump to supposed indent character
	c++;

	if (buf[c] == ' ') {
		puts("Sorry, but commands after a target can only be indented with tabs.");
		return 1;
	}

	// TODO: processCommand processes \t, ' ', @, -, etc
	
	while (buf[c] == '\t') {
		c++;
		int i = 0;
		while (buf[c] != '\n' && buf[c] != '\0') {
			buffer[i] = buf[c];
			c++; i++;
		}

		c++;
		buffer[i] = '\0';
		char *processed = processString(buffer, NULL);
		runCommand(processed);
	}
}

// Allocate end of line value, `asd = value`
char *allocEnd(char buf[], int c) {
	int len = 0;
	while ((buf + c)[len] != '\n') {
		len++;
	}

	// TOOD: handle backslash

	char *s = malloc(len);
	memcpy(s, buf + c, len);
	s[len] = '\0';

	return s;
}

int skipToEnd(char buf[], int c) {
	while (buf[c] != '\n') {
		c++;
	}

	return c;
}

int openFile(char file[]);

int processFile(char buf[]) {
	int line = 0;
	int c = 0;

	int recipeStarted = 0;

	// loop file
	while (1) {
		// loop line
		while (1) {
			// Read into buffer set up when program started
			int b = 0;

			// Ignore spaces, only used in ifdef
			while (buf[c] == ' ') {
				c++;
			}

			if (buf[c] == '\t' && !recipeStarted) {
				puts("Makefiles can only use tabs after targets. If you want to indent ifdefs and stuff, you need to use spaces.");
				return 1;
			}

			// Parse first token, like CC in `CC=gcc`
			while (validChar(buf[c])) {
				buffer[b] = buf[c];
				b++; c++;
				if (b > BUFFER_SIZE) {
					puts("value exceeded size");
					return 1;
				}
			}

			// Skip spacers after token, like in
			// `a = hi`. Don't want "a " as variable name.
			while (buffer[b - 1] == ' ') {
				b--;
			}

			buffer[b] = '\0';

			// Skip spacers, `cc := gcc`
			while (buf[c] == '\t' || buf[c] == ' ') {
				c++;
			}

			if (buf[c] == '=') {
				c++;
				char *name = malloc(strlen(buffer));
				strcpy(name, buffer);
				addMacro(name, allocEnd(buf, c));

				c = skipToEnd(buf, c);
			} else if (buf[c] == ':') {
				recipeStarted = 1;

				c++;
				char *name = malloc(strlen(buffer));
				strcpy(name, buffer);
				addTarget(buf, c, name);
				
				c = skipToEnd(buf, c);
				while (buf[c] == '\t') {
					c = skipToEnd(buf, c);
				}
			}

			// Process statement lines
			if (buf[c] == '\n' || buf[c] == '\0') {
				processString(buffer, NULL);
			}

			if (buf[c] == '\n') {
				line++; c++;
				break;
			}

			if (buf[c] == '\0') {
				return 0;
			}
		}
	}

	return 0;
}

int openFile(char file[]) {
	FILE *fp = fopen(file, "r");

	fseek(fp, 0, SEEK_END);
	int size = (int)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buf = malloc(size);
	fread(buf, size, 1, fp);
	buf[size - 1] = '\0';

	int v = processFile(buf);

	fclose(fp);

	return v;
}

int main(int argc, char *argv[]) {
	// Allocate 10k for loading strings and expansion
	buffer = malloc(BUFFER_SIZE);

	char *file = "Makefile";

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'f':
				i++;
				file = argv[i];
				break;
			case 'v':
				puts("Flake, GPL3.0, Daniel C");
				return 0;
			}
		}
	}

	if (openFile(file)) {
		puts("Error in flake.");
		return 1;
	}

	int noTarget = 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			i++;
		} else {
			if (targetLen == 0) {
				puts("No target to run.");
			} else {
				noTarget = 0;
				runTarget(argv[i]);
			}
		}
	}

	if (noTarget) {
		runTarget(targets[0].name);
	}

	return 0;
}
