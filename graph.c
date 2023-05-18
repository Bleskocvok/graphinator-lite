#include <stdio.h>      // printf, fputs, FILE, fscanf, stderr, stdin
#include <stdlib.h>     // malloc, realloc, free, exit, NULL
#include <assert.h>     // assert
#include <string.h>     // strerror
#include <errno.h>      // errno


typedef struct
{
    unsigned char data[4];

} char_utf8_t;


typedef struct
{
    int align;
    int decimals;

} desc_t;


char_utf8_t get_braille(int col1, int col2)
{
    assert(col1 >= 0);
    assert(col1 <= 4);
    assert(col2 >= 0);
    assert(col2 <= 4);

    char_utf8_t result;

    unsigned char a = 0x28;     // braille start at U+2800
    unsigned char b = 0;

    static const unsigned char masks[] = { 0x4, 0x6, 0x7 };

    if (col1 >= 1)
    {
        b |= 0x1 << 6;

        if (col1 >= 2)
            b |= masks[col1 - 2];
    }

    if (col2 >= 1)
    {
        b |= 0x1 << 7;

        if (col2 >= 2)
            b |= masks[col2 - 2] << 3;
    }

    // UTF-8 prefixes
    result.data[0] = 0xE0;      // 0b11100000
    result.data[1] = 0x80;      // 0b10000000
    result.data[2] = 0x80;      // 0b10000000
    result.data[3] = '\0';      // terminating zero

    // byte 1
    result.data[0] |= (a & 0xF0) >> 4;
    // byte 2
    result.data[1] |= (a & 0x0F) << 2;
    result.data[1] |= (b & 0xC0) >> 6;
    // byte 3
    result.data[2] |= b & 0x3F;

    return result;
}


int get_dots(float row, float value, float max_value, int height)
{
    float chunk = max_value / height;

    if (value <= row * chunk)
        return 0;

    if (value >= (row + 1) * chunk)
        return 4;

    return (value - row * chunk) * 4 / chunk;

    return 4;
}


void spaces(int count)
{
    for (int i = 0; i < count; ++i)
        printf(" ");
}


void render_graph(const float* values, int count, float max_value, int height,
                  const desc_t* desc)
{
    if (desc)
        printf("%*.*f ┐\n", desc->align, desc->decimals, max_value);

    for (int r = height - 1; r >= 0; --r)
    {
        if (desc)
        {
            if (r == height / 2 && height % 2 == 1)
                printf("%*.*f ┤", desc->align, desc->decimals, max_value / 2);
            else
                spaces(desc->align + 1), printf("│");;
        }
        else
        {
            printf("│");
        }

        for (int c = 0; c < count; c += 2)
        {
            int col1 = get_dots(r, values[c],     max_value, height);
            int col2 = c + 1 < count
                     ? get_dots(r, values[c + 1], max_value, height)
                     : 0;

            char_utf8_t braille = get_braille(col1, col2);
            printf("%s", braille.data);
        }
        printf("\n");
    }

    if (desc)
        printf("%*.*f ┴", desc->align, desc->decimals, 0 * max_value);
    else
        printf("└");

    for (int c = 0; c < count; c += 2)
        printf("─");
    printf("\n");
}


float* read_input(FILE* file, int* count_out)
{
    int buf = 256;
    int count = 0;

    float* ptr = (float*) malloc(buf * sizeof(float));

    if (!ptr)
    {
        fputs("ERROR: malloc failed\n", stderr);
        exit(EXIT_FAILURE);
    }

    float num = 0;
    while (fscanf(file, "%f", &num) == 1)
    {
        if (++count > buf)
        {
            buf *= 2;
            ptr = (float*) realloc((void*) ptr, buf * sizeof(float));

            if (!ptr)
            {
                fputs("ERROR: realloc failed\n", stderr);
                exit(EXIT_FAILURE);
            }
        }

        ptr[count - 1] = num;
    }

    if (count_out)
        *count_out = count;

    return ptr;
}


int main(int argc, char** argv)
{
    int height = 7;
    float max_value;

    if (argc >= 2)
    {
        if (sscanf(argv[1], "%d", &height) == 0)
            return fputs("ERROR: invalid height\n", stderr), EXIT_FAILURE;
    }
    else
    {
        return fprintf(stderr, "Usage: %s ‹HEIGHT› [MAX_VALUE]\n", argv[0]),
               EXIT_FAILURE;
    }

    int count = 0;
    float* values = read_input(stdin, &count);

    if (argc >= 3)
    {
        if (sscanf(argv[2], "%f", &max_value) == 0)
            return fputs("ERROR: invalid max_value\n", stderr), EXIT_FAILURE;
    }
    else
    {
        max_value = count > 0 ? values[0] : 0;
        for (int i = 1; i < count; ++i)
            max_value = values[i] > max_value ? values[i] : max_value;
    }

    render_graph(values, count, max_value, height, NULL);

    free(values);

    return EXIT_SUCCESS;
}
