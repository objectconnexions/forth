#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "uart.h"

#define LIT 0x1
#define DUP 0x2
#define SWAP 0x3
#define DROP 0x4

#define ADD 0x10
#define SUBTRACT 0x11
#define DIVIDE 0x12
#define MULTIPLY 0x13
#define GREATER_THAN 0x15
#define EQUAL_TO 0x16
#define AND 0x17
#define OR 0x18
#define XOR 0x19
#define LSHIFT 0x1a
#define RSHIFT 0x1b

#define PRINT_TOS 0xc8
#define PRINT_HEX 0xc9 
#define CR 0xca
#define EMIT 0xcb

#define READ_MEMORY 0xc0
#define WRITE_MEMORY 0xc1

#define RUN 0xe0
#define RETURN 0xe1
#define ZBRANCH 0xe2
#define BRANCH 0xe3

#define WORDS 0xf0
#define STACK 0xf1
#define DEBUG 0xf2
#define TRACE_ON 0xf3
#define TRACE_OFF 0xf4
#define RESET 0xf5
#define CLEAR_REGISTERS 0xf6
#define END 0xf7


#define SCRATCHPAD 64

void dump_stack(void);
void compile(char*, uint8_t*);
void execute(uint8_t*);
void display_code(uint8_t*);
void debug(void);

struct Dictionary {
	struct Dictionary* previous;
	char* name;
	//uint16_t code;
    uint8_t *code;
};

struct Task {
    uint32_t next;
    uint32_t interval;
    //uint16_t code;
};

uint8_t code_store[2048];
uint32_t stack[2048];
uint32_t return_stack[64];

//uint8_t* code_store;
uint8_t* compiled_code;
uint8_t* code;
uint8_t* scratchpad_code;
//int16_t* stack;
//uint16_t* return_stack;
uint16_t ip; // instruction pointer
int sp = 0; // stack pointer
int rsp = 0; // return stack pointer
struct Dictionary* dictionary;
struct Task* tasks;
uint8_t task_count;
bool immediate = false;
bool trace = true;

int forth_init() {
//	code_store = malloc(sizeof(uint8_t) * 2048);
//	stack = malloc(2048);
//	return_stack = malloc(sizeof(uint16_t) * 64);
	scratchpad_code = code_store;
	compiled_code = code_store + SCRATCHPAD;

	code = scratchpad_code;

	sp--;

	uart_transmit("FORTH v0.1\n\n");
	
    /*
    char word[SCRATCHPAD];
	while(1) {
		printf( "> " );
		if (fgets(word, SCRATCHPAD, stdin) != NULL ) {
			word[strlen(word) - 1] = 0;
			if (strcmp(word, "exit") == 0) {
				return 0;
			}
			if (strcmp(word, ">") == 0) {
				dump_stack();
				continue;
			}
			if (strcmp(word, "debug") == 0) {
				debug();
				continue;
			}
			if (strcmp(word, "trace") == 0) {
				trace = true;
				continue;
			}
			if (strcmp(word, "traceoff") == 0) {
				trace = false;;
				continue;
			}
			if (strcmp(word, "reset") == 0) {
				dictionary = NULL;
				//	next_new_word = 0;
				scratchpad_code = code_store;
				compiled_code = code_store + SCRATCHPAD;
				sp = 0;
				rsp = 0;
				ip = 0;
				continue;
			}
			if (strcmp(word, "clear") == 0) {
				sp = 0;
				rsp = 0;
				ip = 0;
				continue;
			}

			if (strncmp(word, "include ", 8) == 0) {
				FILE *stream;
				char *line = NULL;
				size_t len = 0;
				ssize_t read;
				char filepath[20];
				strcpy(filepath, word + 8);
				len = strlen(filepath);
				if (strcspn(filepath, ".") == len) {
					strcat(filepath, ".fth");
				}
				stream = fopen(filepath, "r");
				if (stream == NULL) {
					printf ("failed to open file %s", filepath);
					continue;
				}

				memset(code_store, 0, SCRATCHPAD);
				while ((read = getline(&line, &len, stream)) != -1) {
					printf("Retrieved line of length %zu :\n", read);
					if (*(line + read -1) == '\n') {
						read--;
						*(line + read) = 0;
					}
					if ( read == 0) {
						continue;
					}
					printf("'%s'\n", line);

					compile(line, code_store);
				}

				free(line);
				fclose(stream);

				ip = 0;
				execute(code_store);
				dump_stack();

				continue;
			}

			if (strcmp(word, "") == 0) {
				continue;
			}
			memset(code_store, 0, SCRATCHPAD);
			scratchpad_code = code_store;
			compile(word, code_store);
			printf("entered '%s'\n", word);
			ip = 0;
			execute(code_store);
			scratchpad_code = code_store;
			code = scratchpad_code;
			dump_stack();
		}
	}


	/*





  uint8_t* program = code_store;



  // DUP + 3 * . CR
  int i = 0;

  program[i++] = LIT;
  program[i++] = 10;
  program[i++] = DUP;
  program[i++] = PLUS;
  program[i++] = LIT;
  program[i++] = 3;
  program[i++] = MULTIPLY;
  program[i++] = DOT;
  program[i++] = CR;
  program[i++] = END;


  //stack[0] = 11;
  //  stack[1] = 2;
  //  sp = 1;

  //  char* source = ": DOUBLE DUP + ; 3 DOUBLE .\nCR\nEND";
  //  char* source = "0 IF 10 ELSE 1 IF 33 THEN ELSE THEN . 2 3 > STACK IF 40 ELSE 50 THEN . CR END";
  char* source = "CR 99 2 3 STACK + CR . CR CR END";

  compile(source, code_store);
  display_code(code_store);

  program = code_store;
	 */
}

void forth_execute(uint8_t* word) {
    if (trace) {
        printf("# input %s\n", word);
    }
    if (!immediate) {
        memset(code_store, 0, SCRATCHPAD);
        scratchpad_code = code_store;
    }
    compile(word, code_store);
    
    if (!immediate) {
        ip = 0;
        execute(code_store);
        scratchpad_code = code_store;
        code = scratchpad_code;
        dump_stack();
    }
}

void forth_tasks(uint32_t ticks) {
    int i;
    for (i = 0; i < task_count; i++) {
        struct Task task = tasks[i];

        if (ticks >= task.next) {
     //       printf("task from %i\n", task.code);
//            return_stack[++rsp] = ip + 1;
//            uint8_t code_at = code[ip ++];
//            ip = code_at + SCRATCHPAD;
   //         execute(task.code);
            
            task.next += task.interval;
        }

    }

}

void words() {
    struct Dictionary * ptr = dictionary;
    while (ptr != NULL) {
        printf("%s -> %04x\n", ptr->name, ptr->code + SCRATCHPAD);
        ptr = ptr->previous;
    }
}

void execute(uint8_t* code) {
	uint32_t tos_value;
	uint32_t nos_value;
    
	while(1) {

		uint8_t instruction = code[ip++];

		if (instruction == 0) {
			return;
		}

		if (trace) {
			printf("execute @%i [%x|%x]\n", ip - 1, instruction, code[ip]);
		}

		switch (instruction) {
		case DUP:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp];
			stack[++sp] = tos_value;
			break;

		case DROP:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			sp--;
			break;

		case SWAP:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp];
			stack[sp] = stack[sp -1];
			stack[sp - 1] = tos_value;
			break;

		case LIT:
			tos_value = code[ip++] +
                    code[ip++] * 0x100 +
                    code[ip++] * 0x10000 +
                    code[ip++] * 0x1000000;
			stack[++sp] = tos_value;
			break;

		case ADD:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value + tos_value;
			stack[++sp] = tos_value;
			break;

		case SUBTRACT:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value - tos_value;
			stack[++sp] = tos_value;
			break;

		case DIVIDE:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value / tos_value;
			stack[++sp] = tos_value;
			break;

		case MULTIPLY:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value * tos_value;
			stack[++sp] = tos_value;
			break;

		case GREATER_THAN:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value > tos_value ? 1 : 01;
			stack[++sp] = tos_value;
			break;

		case EQUAL_TO:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value == tos_value ? 1 : 0;
			stack[++sp] = tos_value;
			break;

		case AND:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value &tos_value;
			stack[++sp] = tos_value;
			break;

		case OR:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value | tos_value;
			stack[++sp] = tos_value;
			break;
            
		case XOR:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value ^ tos_value;
			stack[++sp] = tos_value;
			break;

        case LSHIFT:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value << tos_value;
			stack[++sp] = tos_value;
			break;

        case RSHIFT:
			if (sp < 1) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			nos_value = stack[sp--];
			tos_value = nos_value >> tos_value;
			stack[++sp] = tos_value;
			break;
            
		case ZBRANCH:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			if (trace) {
				printf("zbranch for %i -> %s\n", tos_value, (tos_value == 0 ? "zero" : "non-zero"));
			}
			if (tos_value == 0) {
				int relative = code[ip];
				if (relative > 128) {
					relative = relative - 256;
				}
				if (trace) {
					printf("jump %i\n", relative);
				}
				ip = ip + relative;
			} else {
				ip++;
			}
			break;

		case BRANCH:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			int relative = code[ip];
			if (relative > 128) {
				relative = relative - 256;
			}
			if (trace) {
				printf("jump %i\n", relative);
			}
			ip = ip + relative;
			break;

		case RUN:
			printf("run from %i\n", code[ip] + SCRATCHPAD);
			return_stack[++rsp] = ip + 1;
			uint8_t code_at = code[ip ++];
			ip = code_at + SCRATCHPAD;
			break;

		case RETURN:
			ip = return_stack[rsp--];
			break;

		case PRINT_TOS:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
			tos_value = stack[sp--];
			printf("%i ", tos_value);
			break;

		case CR:
			printf("\n");
			break;

		case EMIT:
			if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}
            tos_value = stack[sp--];
			printf("%c", tos_value);
			break;

        case READ_MEMORY:
            if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}

            tos_value = stack[sp--];                        
            printf("address 0x%x\n", tos_value);
            uint32_t *ptr = (uint32_t *) tos_value;
            tos_value = (uint32_t) *ptr;
            printf("value 0x%x\n", tos_value);
            stack[++sp] = tos_value;
            break;

        case WRITE_MEMORY:
            if (sp < 0) {
				printf("stack underflow; aborting\n");
				return;
			}

            tos_value = stack[sp--];
            nos_value = stack[sp--];
            printf("set 0x%x to 0x%x\n", nos_value, tos_value);
            ptr = (uint32_t *) nos_value;
            *ptr = tos_value;
            break;

        case PRINT_HEX:
            tos_value = stack[sp--];
            printf("0x%x", tos_value);
            break;
            
        case WORDS:
            words();
            break;
            
		case STACK:
			dump_stack();
			break;

		case DEBUG:
            debug();
			break;

		case TRACE_ON:
            trace = true;
            break;

		case TRACE_OFF:
            trace = false;;
			break;

		case RESET:
            dictionary = NULL;
            //	next_new_word = 0;
            scratchpad_code = code_store;
            compiled_code = code_store + SCRATCHPAD;
            sp = 0;
            rsp = 0;
            ip = 0;
            scratchpad_code[ip] = END;
            break;

		case CLEAR_REGISTERS:
            sp = 0;
            rsp = 0;
            ip = 0;
            break;

		case END:
			dump_stack();
			return;
			break;

		default:
			printf("unknown instruction %i", instruction);
			return;
			break;
		}
	}
}

void dump_stack() {
	if (sp >= 0) {
		printf("< ");
        int i;
		for (i = 0; i <= sp; i++) {
			if (i == sp) {
				printf("| %i >", stack[i]);
			} else {
				printf("%i ", stack[i]);
			}
		}
	} else {
		printf("< >");
	}
	printf("\n");
}



uint8_t *jumps[6];
uint8_t jp = 0;
// uint8_t* define_start;


void to_upper(char *string, int len) {
    int i;
    for (i = 0; i <= len; i++) {
        if (*string >= 97 && *string <= 122) {
            *string = *string - 32;
        }
        string++;
    }
}

void compile(char* source, uint8_t* ignore) {
	char* start = source;

	if (trace) {
		printf("compile [%s]\n", source);
	}


	while (true) {
		while (*source == ' ' || *source == '\t') {
			source++;
			start = source;
		}
		if (*source == 0 /* || *source == '\n' */) {
			return;
		}

		while (*source != 0 && *source != ' ' && *source != '\t') {
			source++;
		}


		char instruction[16];
		int len = source - start;
		/*
			if (c == 0) {
				source++;
				continue;
			}
		 */
		strncpy(instruction, start, len);
		instruction[len] = 0;

        to_upper(instruction, strlen(instruction));

		if (trace) {
			printf("instruction [%s] %i\n", instruction, len);
		}

		if (strcmp(instruction, "STACK") == 0) {
			*code++ = STACK;
            
		} else if (strcmp(instruction, "DUP") == 0) {
			*code++ = DUP;
            
		} else if (strcmp(instruction, "SWAP") == 0) {
			*code++ = SWAP;
            
		} else if (strcmp(instruction, "DROP") == 0) {
			*code++ = DROP;
            
		} else if(strcmp(instruction, "END") == 0) {
			*code++ = END;
            
		} else if(strcmp(instruction, "CR") == 0) {
			*code++ = CR;

		} else if(strcmp(instruction, "EMIT") == 0) {
			*code++ = EMIT;
            
        } else if(strcmp(instruction, "IF") == 0) {
			*code++ = ZBRANCH;
			jumps[jp++] = code;
			code++;
            
		} else if (strcmp(instruction, "THEN") == 0) {
			uint8_t distance = code - jumps[--jp];
			*(jumps[jp]) = distance;
            
		} else if(strcmp(instruction, "BEGIN") == 0) {
			jumps[jp++] = code;
            
		} else if (strcmp(instruction, "UNTIL") == 0) {
			*code++ = ZBRANCH;
			uint8_t distance = jumps[--jp] - code;
			*code++ = distance;
            
		} else if(strcmp(instruction, ":") == 0) {
			start = source + 1;
			while (*++source != ' ' && *source != 0) {}
			int len = source - start;
			struct Dictionary* entry = malloc(sizeof(struct Dictionary));
			entry->code = compiled_code - (code_store + SCRATCHPAD);
			entry->name = malloc(len + 1);
			entry->previous = dictionary;
            to_upper(start, len);
			strncpy(entry->name, start, len);
			dictionary = entry;
			scratchpad_code = code;
			code = compiled_code;
            
            immediate = true;
            
		} else if(strcmp(instruction, ";") == 0) {
			*code++ = RETURN;
			compiled_code = code;
			code = scratchpad_code;
            
            immediate = false;

		} else if(strcmp(instruction, "task") == 0) {
			start = source + 1;
			while (*++source != ' ' && *source != 0) {}
			int len = source - start;
            
			struct Task entry; // = malloc(sizeof(struct Task));
//			entry->code = compiled_code - (code_store + SCRATCHPAD);
//			entry->name = malloc(len + 1);
//			entry->previous = dictionary;
//            to_upper(start, len);
//			strncpy(entry->name, start, len);
			tasks[task_count++] = entry;
			scratchpad_code = code;
			code = compiled_code;
            
            immediate = true;
            
            
		} else if(strcmp(instruction, "+") == 0) {
			*code++ = ADD;
                       
		} else if(strcmp(instruction, "@") == 0) {
			*code++ = READ_MEMORY;
           
		} else if(strcmp(instruction, "!") == 0) {
			*code++ = WRITE_MEMORY;

		} else if(strcmp(instruction, "-") == 0) {
			*code++ = SUBTRACT;
            
		} else if(strcmp(instruction, "/") == 0) {
			*code++ = DIVIDE;
            
		} else if(strcmp(instruction, "*") == 0) {
			*code++ = MULTIPLY;
            
		} else if(strcmp(instruction, ">") == 0) {
			*code++ = GREATER_THAN;
            
		} else if(strcmp(instruction, "=") == 0) {
			*code++ = EQUAL_TO;
            
        } else if(strcmp(instruction, "AND") == 0) {
			*code++ = AND;
            
        } else if(strcmp(instruction, "OR") == 0) {
			*code++ = OR;
            
        } else if(strcmp(instruction, "XOR") == 0) {
			*code++ = XOR;
            
        } else if(strcmp(instruction, "LSHIFT") == 0) {
			*code++ = LSHIFT;
            
        } else if(strcmp(instruction, "RSHIFT") == 0) {
			*code++ = RSHIFT;
            
		} else if(strcmp(instruction, ".") == 0) {
			*code++ = PRINT_TOS;

		} else if(strcmp(instruction, ".HEX") == 0) {
			*code++ = PRINT_HEX;

		} else if(strcmp(instruction, "WORDS") == 0) {
            *code++ = WORDS;
            
		} else if(strcmp(instruction, "_DEBUG") == 0) {
            *code++ = DEBUG;

		} else if(strcmp(instruction, "_TRACE") == 0) {
            *code++ = TRACE_ON;

		} else if(strcmp(instruction, "_NOTRACE") == 0) {
            *code++ = TRACE_OFF;

		} else if(strcmp(instruction, "_RESET") == 0) {
            *code++ = RESET;

		} else if(strcmp(instruction, "_CLEAR") == 0) {
            *code++ = CLEAR_REGISTERS;          
            
		} else {
           // strncpy(instruction, start, len);
           // instruction[len] = 0;        
			struct Dictionary * ptr = dictionary;
			while (ptr != NULL) {
				if (strcmp(instruction, ptr->name) == 0) {
					if (trace) {
						printf("# run %s at %i\n", ptr->name, ptr->code + SCRATCHPAD);
					}
					*code++ = RUN;
					*code++ = ptr->code;
					break;
				}
				ptr = ptr->previous;
			}

			if (ptr == NULL) {
				bool is_number = true;
                int i;
				for (i = 0; i < strlen(instruction); ++i) {
					if (!isdigit(instruction[i]) && i == 2 && !(instruction[i] == 'X' || instruction[i] == 'B')) {
						is_number = false;
						break;
					}
				}
				if (is_number) {
                    int64_t value = strtoll(instruction, NULL, 0);
					*code++ = LIT;
                    if (trace) {
                        printf("# literal = 0x%09llx\n", value);
                    }
					*code++ = value % 0x100;
					*code++ = (value / 0x100) % 0x100;
					*code++ = (value / 0x10000) % 0x100;
					*code++ = (value / 0x1000000) % 0x100;
				}
			}
		}
	}
	source++;
}

void debug() {
    int i;

	if (dictionary == NULL) {
		printf("No new words\n");
	} else {
        words();
	}

/*
	printf ("\n");
	for (i = 0; i < SCRATCHPAD; i++) {
		printf("[%03i] ", code_store[i]);

		if (i % 16 == 15) {
			printf("\n");
		}

	}
	printf("\n");
*/
	printf ("\n     ");
	for (i = 0; i < 16; i++) {
		printf("%3x ", i);
	}
	printf ("\n");
	for (i = 0; i < 512; i++) {
		if (i % 16 == 0) {
			printf("\n %5x  ", i);
		}
		printf("%3x ", code_store[i]);

	}
	printf("\n");

    for (i = 0; i < task_count; i++) {
        struct Task task = tasks[i];
//        printf("task#%i - %i, %i - %04x\n", i + 1, task.interval, task.next, task.code);
    }

	printf ("\n");
}

void display_code(uint8_t* code) {

}
