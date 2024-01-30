#include <string.h>


void to_upper(char *string)
{
    int len = strlen(string);
    int i;
    for (i = 0; i <= len; i++) {
        if (*string >= 'a' && *string <= 'z') {
            *string = *string - 32;
        }
        string++;
    }
}
