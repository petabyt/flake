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
	{"IS_FLAKE", "0.1.0"},
	{"OS",
	#ifdef WIN32
		"Windows"
	#else
		"Linux"
	#endif
	}
};
int macroLen = 1;

// Structure to hold rules
#define MAX_RULE 100
struct Rules {
	char *name;
	char *buf;
	int pos;
}rules[MAX_RULE];
int ruleLen = 0;

// Assumes all parameters are allocated
// TODO: detect if already exist
void addMacro(char name[], char value[]) {
	macros[macroLen].name = name;
	macros[macroLen].value = value;
	macroLen++;
}

// Also assumes parameters are allocated
void addRule(char buf[], int pos, char name[]) {
	rules[ruleLen].name = name;
	rules[ruleLen].pos = pos;
	rules[ruleLen].buf = buf;
	ruleLen++;
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
	return (c != '?' && c != ':' && c != '=' && c != '\0' && c != '\n');
}

// Wildcard comparasion
// Inspired by tutorialspoint
int wldcmp(char *first, char *second, char cmp) {
	if (*first == '\0' && *second == '\0') {
		return 1;
	}

	if (*first == cmp && *(first + 1) != '\0' && *second == '\0') {
		return 0;
	}
 
	// If the first string contains '?', or current characters
	// of both strings match
	if (*first == '?' || *first == *second) {
		return wldcmp(first + 1, second + 1, cmp);
	}
 
	if (*first == cmp) {
		return wldcmp(first + 1, second, cmp) || wldcmp(first, second + 1, cmp);
	}

	return 0;
}

// Use processed to tell whether all macros in string
// have been implemented. Can be NULL and won't do it
char *processString(char string[]) {
	char *buf = malloc(MAX_STRING);
	buf[0] = '\0';

	int processed = 1;

	for (int c = 0; string[c] != '\0'; c++) {
		if (string[c] == '$') {
			c++;

			// Basic processing stack. processString will
			// be called recursively, for nested macros
			int stack = 1;
			if (string[c] == '(' || string[c] == '{') {
				processed = 0;

				c++;
				int origin = c;

				// Skip space before stuff $( info asd)
				while (string[c] == ' ' || string[c] == '\t') {c++;}

				// Check to make sure no excess (){}
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

				// Process some basic functions
				// TODO: function parser, parameter parser
				int c2 = 0;
				if (!strncmp(newString, "info", 4)) {
					c2 += 4;
					while (newString[c2] == ' ' || newString[c2] == '\t') {c2++;}
					puts(processString(newString + c2));
				} else if (!strncmp(newString, "shell", 4)) {
					c2 += 5;
					while (newString[c2] == ' ' || newString[c2] == '\t') {c2++;}
					FILE *cmd = popen(processString(newString + c2), "r");

					char *read = malloc(MAX_STRING);
					fread(read, 1, MAX_STRING, cmd);
					fclose(cmd);

					// GNU make seems to replace \n with spaces, hold off for now
					for (int i = 0; read[i] != '\0'; i++) {
						if (read[i] == '\n') {
							read[i] = ' ';
							if (read[i + 1] == '\0') {
								read[i] = '\0';
							}
						}
					}

					strcat(buf, read);
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

	char *process = processString(command);

	if (!silent) {
		puts(process);
	}

	int ret = system(process);
	if (noerror) {
		return 0;
	}

	return ret;
}

int runTarget(char name[]) {
	// TODO: process wildcard target logic
	// TODO: process spaces between targets
	// TODO: return bool if target is completed
	int i;
	for (i = 0; i < ruleLen; i++) {
		if (!strcmp(rules[i].name, name)) {
			goto found;
			return 0;
		}
	}

	printf("Error: No rule to make target %s\n", name);
	return 1;

	found:;

	// Go back to where processFile left off
	char *buf = rules[i].buf;
	int c = rules[i].pos;

	while (buf[c] == ' ') {c++;}

	// Scan through prerequisities first
	// temporary, entire thing should be processed as macro
	char *preq = malloc(MAX_TARGET_STRING);
	int preqC = 0;
	while (buf[c] != '\n') {
		if (validChar(buf[c])) {
			if (buf[c] == ' ') {
				// Process the target between spaces
				preq[preqC] = '\0'; preqC = 0;
				printf("%s\n", preq);
				runTarget(preq);
				while (buf[c] == ' ') {c++;}
			} else {
				preq[preqC] = buf[c];
				preqC++;
			}
		}

		c++;
	}

	// Run last target if found, since the loop cut it off
	preq[preqC] = '\0';
	if (preqC != 0) {
		runTarget(preq);
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
	// GNU Make likes to remove first tabs and spaces
	while (buf[c] == ' ' || buf[c] == '\t') {
		c++;
	}

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
			puts("Error: Tabs can only be used after rules.");
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
		while (buffer[b - 1] == ' ' || buffer[b - 1] == '\t') {
			b--;
		}

		buffer[b] = '\0';

		// Skip spacers
		while (buf[c] == '\t' || buf[c] == ' ') {
			c++;
		}

		if (!strcmp(buffer, "ifdef")) {
			
		}

		if (buf[c] == '=') {
			c++;

			char *name = malloc(strlen(buffer));
			strcpy(name, buffer);
			addMacro(name, processString(allocEnd(buf, c)));

			c = skipToEnd(buf, c);
		} else if (buf[c] == '?') {			
			c += 2;
			char *name = malloc(strlen(buffer));
			strcpy(name, buffer);

			if (getMacro(name) == nothing) {
				addMacro(name, allocEnd(buf, c));
			}

			c = skipToEnd(buf, c); 
		} else if (buf[c] == ':' && buf[c + 1] == '=') {
			c += 2;
			char *name = malloc(strlen(buffer));
			strcpy(name, buffer);

			addMacro(name, allocEnd(buf, c));
			c = skipToEnd(buf, c);
		} else if (buf[c] == ':') {
			recipeStarted = 1;

			c++;
			char *name = malloc(strlen(buffer));
			strcpy(name, buffer);
			addRule(buf, c, name);
			
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
	if (fp == NULL) {
		printf("Error: %s doesn't exist.\n", file);
		exit(1);
	}

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

		for (int c = 0; argv[i][c] != '\0'; c++) {
			if (argv[i][c] == '=') {
				char *name = malloc(c);
				strncpy(name, argv[i], c);
				name[c] = '\0';

				c++;

				addMacro(name, argv[i] + c);

				i++;
				break;
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
		} else if (strchr(argv[i], '=') != NULL) {
			i++;
		}  else {
			if (ruleLen == 0) {
				puts("Error: No target to run.");
			} else {
				noTarget = 0;
				runTarget(argv[i]);
			}
		}
	}

	if (noTarget) {
		runTarget(rules[0].name);
	}

	return 0;
}
