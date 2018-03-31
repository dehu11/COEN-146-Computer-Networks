#include <stdio.h>

int main ( int argc, char*argv[]) {
        if (argc != 3) {
                return -1;
        }

        FILE *src, *dest;
        size_t stuff;
        char buffer[10];

        src = fopen(argv[1], "rb");
        dest = fopen(argv[2], "wb");

        while ((stuff = fread(buffer, 1, 10, src)) != 0) {
                if (fwrite(buffer, 1, stuff, dest) != stuff) {
                        return 1;
                }
        }

        fclose(src);
        fclose(dest);
}
