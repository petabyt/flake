#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum value for target names
#define MAX_TARGET_STRING 1024

// Max value for string expansion
#define MAX_STRING 10 * 1024

char *nothing = "";

int line = 0;

// Hold macro addresses and names
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

// Structure to hold targets
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
char *processString(char string[]) {
	// TODO: allocate bigger buffer?
	char *buf = malloc(MAX_STRING);
	buf[0] = '\0';

	int processed = 1;

	for (int c = 0; string[c] != '\0'; c++) {
		if (string[c] == '$') {
			processed = 0;
			c++;

			// Basic processing stack. processString will
			// be called recursively, for nested macros
			int stack = 1;
			if (string[c] == '(' || string[c] == '{') {
				c++;
				int origin = c;

				// Skip space before stuff $( info asd)
				while (string[c] == ' ' || string[c] == '\t') {c++;}

				while (string[c] != '\n') {
					if (string[c] == '('  || string[c] == '{') {
						stack++;
					} else if (string[c] == ')'  || string[c] == '}') {
						stack--;
					} else if (string[c] == '\0') {
						printf("Error: Expected an ending ')' while parsing '%s'.\n", string);
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
					puts(processString(newString + c2));
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

	if (!processed) {
		return processString(buf);
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

	printf("Error: No rule to make target %s\n", name);
	return 1;

	found:;


	char *preq = malloc(MAX_TARGET_STRING);
	int preqC = 0;

	// Skip to start of 
	char *buf = targets[i].buf;
	int c = targets[i].pos;
	while (buf[c] != '\n') {
		if (validChar(buf[c]) && buf[c] != ' ') {
			preq[preqC] = '\0';
		}

		c++;
	}

	// Jump to supposed indent character
	c++;

	if (buf[c] == ' ') {
		printf("%s\n", "Error: Commands after a target can only be indented with tabs.");
		return 1;
	}

	// Process each command after target
	char *buffer = malloc(MAX_STRING);
	while (buf[c] == '\t') {
		c++;
		int i = 0;
		while (buf[c] != '\n' && buf[c] != '\0') {
			buffer[i] = buf[c];
			c++; i++;
		}

		c++;
		buffer[i] = '\0';
		char *processed = processString(buffer);
		runCommand(processed);
	}
}

// Allocate end of line value, `asd = value`
char *allocEnd(char buf[], int c) {
	int len = 0;
	while ((buf + c)[len] != '\n') {
		len++;
	}

	// TODO: handle backslash

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

	// Loop each line
	char *buffer = malloc(MAX_STRING);
	while (1) {
		int b = 0;

		// Ignore spaces, only used in ifdef
		while (buf[c] == ' ') {
			c++;
		}

		if (buf[c] == '\t' && !recipeStarted) {
			puts("Error: Tabs can only be used after targets.");
			return 1;
		}

		// Parse first token, like CC in `CC=gcc`
		while (validChar(buf[c])) {
			buffer[b] = buf[c];
			b++; c++;
			if (b > MAX_STRING) {
				puts("Error: Line exceeded MAX_STRING");
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
			processString(buffer);
		}

		if (buf[c] == '\n') {
			line++; c++;
		}

		if (buf[c] == '\0') {
			return 0;
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
	char *file = "Makefile";

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'f':
				i++;
				file = argv[i];
				break;
			case 'v':
				puts("Flake - GPL3.0 - Daniel C");
				return 0;
			}
		}
	}

	if (openFile(file)) {
		return 1;
	}

	int noTarget = 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			i++;
		} else {
			if (targetLen == 0) {
				puts("Error: No target to run.");
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
