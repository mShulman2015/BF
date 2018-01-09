#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

void print_help();

typedef struct paren {
    char *start;
    struct paren *prev;
    struct paren *next;
} paren;

char* hello_instrctions = "++++++++++[>+++++++>++++++++++>+++<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------."; // hello world

int main(int argc, char** argv) {
    char* ip = hello_instrctions; // instruction pointer

    short allow_incrementing_array = 0;
    int start_length_of_data = 30000;
    int max_length_of_data = 30000;

    int c;
    while((c = getopt(argc, argv, "hei:f:")) != -1) {
        switch(c) {
            case 'h':
                print_help(argv[0]);
                exit(0);

            case 'e':
                allow_incrementing_array = 1;
                break;
            case 'i':
                start_length_of_data = atoi(optarg);
                break;
            case 'f':
                max_length_of_data = atoi(optarg);
                break;

            case '?':
                fprintf(stderr, "something fucked up\n");
                exit(-1);

            default:
                fprintf(stderr, "argument parsing has gone wrong, please report the error the the creator of the module\n");
                exit(-1);
        }
    }
    // addjust arg array to read in options
    argc -= optind;
    argv += optind;
    if(argc == 0) {
        ; // don't have to do anything just
    } else if (argc == 1) {
        FILE *file = fopen(argv[0], "r");
        if(file == NULL) {
            fprintf(stderr, "there was an error reading your instruction file: %s\n", argv[0]);
            exit(1);
        }
        fseek(file, 0L, SEEK_END);
        int file_size = ftell(file);
        rewind(file);

        int fd = fileno(file);
        ip = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
        if(ip == MAP_FAILED) {
            fprintf(stderr, "there was an error loading instructions into memory\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "please specify only one code file\n");
        exit(1);
    }


    // parentheses stack
    paren *head = NULL;
    paren *tail = NULL;

    int curr_length_of_data = start_length_of_data;
    unsigned char *data = calloc(sizeof(char), curr_length_of_data);
    if(data == NULL) {
        fprintf(stderr, "couldn't allocate enough memory for data\n");
        exit(1);
    }
    int data_index = 0; // to be called data pointer but will work as an index for implementation purposes

    int skipping = 0; // 0 for when executing instruction inside parens, 1 for when trying to find a matching parenthesis
    int depth = 0;  // amount of parenthesis deep that we're in when trying to skip

    while (*ip != '\0' || *ip == EOF) {

        // check if we are trying to locate parentheses
        if(skipping == 1) {
            if(*ip == '[') {
                depth++;
            } else if(*ip == ']') {
                depth--;
                if(depth == 0) {
                    skipping = 0;
                }
            }
            ip++;
            continue;
        }

        if(*ip == '+') { // increment data value
            data[data_index]++;
        } else if (*ip == '-') { // decrement data value
            data[data_index]--;
        } else if (*ip == '>') { // move the data pointer to the right one
            data_index++;
            if(data_index == curr_length_of_data) {
                if(!allow_incrementing_array) {
                    fprintf(stderr, "Data pointer as reached an invalid location\n");
                    exit(1);
                }
                if(curr_length_of_data >= max_length_of_data) {
                    fprintf(stderr, "Data pointer as reached a value larger than the maximum list size\n");
                    exit(1);
                }

                // we need to increase the size of the array and continue execution
                int new_data_size = (curr_length_of_data * 2 <= max_length_of_data)? curr_length_of_data * 2: max_length_of_data;
                char *new_data = calloc(sizeof(char), new_data_size);
                memcpy(new_data, data, curr_length_of_data);

                curr_length_of_data = new_data_size;
                data = new_data;
            }
        } else if (*ip == '<') { // move the data pointer to the left one
            data_index--;
            if(data_index < 0) {
                fprintf(stderr, "Data pointer has reached a negative value\n");
                exit(1);
            }
        } else if (*ip == '.') { // print current data value to stdout as a char
            printf("%c", data[data_index]);
        } else if (*ip == ',') { // get value from stdout and place it in current data locations
            scanf(" %c", (char *)(data + data_index));
        } else if (*ip == '[') { // if data value is 0 skip to matching paren otherwise execute code inside as normal
            if(data[data_index] == 0) {
                skipping = 1;
                depth = 1;
            } else {
                paren *p = (paren *)malloc(sizeof(paren));
                p->start = ip;
                p->next = NULL;
                p->prev = tail;

                if(head == NULL) {
                    head = p;
                } else {
                    tail->next = p;
                }
                tail = p;
            }
        } else if (*ip == ']') { // if data value is non 0 skip to matching opening paren otherwise continue
            if(head == NULL) {
                fprintf(stderr, "An extra closing paren  was found\n");
                exit(1);
            }

            if(data[data_index] == 0) {
                if(head == tail) {
                    free(head);
                    head = NULL;
                    tail = NULL;
                } else {
                    tail = tail->prev;
                    free(tail->next);
                }
            } else {
                ip = tail->start; //incremented at the end, so the the paren isn't re-checked
            }
        }
        ip++;
    }

    if(head != NULL || skipping == 1) {
        fprintf(stderr, "End of execution was reached, and not enough closing parens where found\n");
        exit(1);
    } else {
        exit(0);
    }
}

void print_help(char *exec_name) {
    printf("Usage: %s [-h] [-e] [-i inicial_data_length] [-f final_data_length] [file_path]\n", exec_name);
    printf("\n");
    printf("-h\tprint help information  displayed here\n");
    printf("-e\tallow for data array to be extended when filled up until it reaches final_data_length\n");
    printf("-i\tset initial size of data array.  Default to 30,000\n");
    printf("-f\tset final size of data array after which expanding is not allowed, only relevant if extension is enabled\n");
    printf("file_path:\tfile to read instructions from\n");
}
