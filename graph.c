#include "graph.h"

#include <stdio.h>      // printf, fputs, FILE, fscanf, stderr, stdin
#include <stdlib.h>     // malloc, realloc, free, exit, NULL
#include <assert.h>     // assert
#include <string.h>     // strerror, strncpy, strcmp
#include <errno.h>      // errno


typedef struct
{
    unsigned char data[4];

} char_utf8_t;


typedef char_utf8_t (*print_char_f) (int c, int r, const double* values,
                                     int count, double max_value, int height);


static char_utf8_t get_braille(int col1, int col2)
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


static int get_level(int levels, double row, double value, double max_value, int height)
{
    double chunk = max_value / height;

    if (value <= row * chunk)
        return 0;

    if (value >= (row + 1) * chunk)
        return levels;

    return (value - row * chunk) * levels / chunk;
}


static void spaces(int count)
{
    for (int i = 0; i < count; ++i)
        printf(" ");
}

static char_utf8_t get_slope(int col1, int col2,
                             int top1, int top2,
                             int bot1, int bot2)
{
    static const char* m[4][4] =
    {
        { " ", "🬽", "🬿", "🮟" },

        { "🭈", "▃", "🭑", "🭏" },

        { "🭊", "🭆", "▆", "🭍" },

        { "🮞", "🭄", "🭂", "█" },
    };

    static const char* m2[4][4] =
    {
        { 0x0, 0x0, 0x0, "▌" },

        { 0x0, 0x0, 0x0, "🭎" },

        { 0x0, 0x0, 0x0, "🭌" },

        { "▐", "🭃", "🭁", "█" },
    };

    static const char* m3[4][4] =
    {
        { " ", "🬼", "🬾", "🭀" },

        { "🭇", 0x0, 0x0, 0x0 },

        { "🭉", 0x0, 0x0, 0x0 },

        { "🭋", 0x0, 0x0, 0x0 },
    };

    char_utf8_t c = { 0 };

    if      (col1 == 0 && col2 == 3) strncpy(c.data, (top2 == 0 ? m3 : m2)[col2][col1], 4);
    else if (col1 == 3 && col2 == 0) strncpy(c.data, (top1 == 0 ? m3 : m2)[col2][col1], 4);
    else if (col1 == 0 && bot1 != 3) strncpy(c.data, m3[col2][col1], 4);
    else if (col1 == 3 && top1 != 0) strncpy(c.data, m2[col2][col1], 4);
    else if (col2 == 0 && bot2 != 3) strncpy(c.data, m3[col2][col1], 4);
    else if (col2 == 3 && top2 != 0) strncpy(c.data, m2[col2][col1], 4);
    else                             strncpy(c.data, m[col2][col1], 4);

    return c;
}


char_utf8_t draw_braille(int c, int r, const double* values,
                         int count, double max_value, int height)
{
    char_utf8_t empty = { 0 };
    if (c % 2) return empty;

    int col1 = get_level(4, r, values[c],     max_value, height);
    int col2 = c + 1 < count
             ? get_level(4, r, values[c + 1], max_value, height)
             : 0;

    return get_braille(col1, col2);
}


char_utf8_t draw_legacy(int c, int r, const double* values,
                        int count, double max_value, int height)
{
    int top1 = 0, top2 = 0,
        bot1 = 0, bot2 = 0;

    if (r < height - 1)
    {
        top1 =               get_level(3, r+1, values[c],   max_value, height);
        top2 = c+1 < count ? get_level(3, r+1, values[c+1], max_value, height) : 0;
    }

    if (r > 0)
    {
        bot1 =               get_level(3, r-1, values[c],   max_value, height);
        bot2 = c+1 < count ? get_level(3, r-1, values[c+1], max_value, height) : 0;
    }

    int col1 =               get_level(3, r, values[c],   max_value, height);
    int col2 = c+1 < count ? get_level(3, r, values[c+1], max_value, height) : 0;

    return get_slope(col1, col2, top1, top2, bot1, bot2);
}


void render_graph_f(const double* values, int count, double max_value,
                    int height, int width, const desc_t* desc,
                    print_char_f draw_func)
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
            printf("│");

        for (int c = 0; c < count; ++c)
        {
            char_utf8_t symbol = draw_func(c, r, values, count, max_value, height);
            printf("%s", symbol.data);
        }
        printf("\n");
    }

    if (desc)
        printf("%*.*f ┴", desc->align, desc->decimals, 0 * max_value);
    else
        printf("└");

    for (int c = 0; c < width; ++c)
        printf("─");
    printf("\n");
}

void render_graph(const double* values, int count, double max_value, int height,
                  const desc_t* desc, style_t style)
{
    if (style == DOTS)
        render_graph_f(values, count, max_value, height, count / 2, desc, draw_braille);
    else if (style == FILL)
        render_graph_f(values, count, max_value, height, count, desc, draw_legacy);
    else
        assert(0);
}


double* read_input(FILE* file, int* count_out)
{
    int buf = 256;
    int count = 0;

    double* ptr = (double*) malloc(buf * sizeof(double));

    if (!ptr)
    {
        fputs("ERROR: malloc failed\n", stderr);
        exit(EXIT_FAILURE);
    }

    double num = 0;
    while (fscanf(file, "%lf", &num) == 1)
    {
        if (++count > buf)
        {
            buf *= 2;
            ptr = (double*) realloc((void*) ptr, buf * sizeof(double));

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

int style_from_str(const char* str, style_t* style)
{
    if (strcmp(str, "dots") == 0) *style = DOTS;
    else if (strcmp(str, "fill") == 0) *style = FILL;
    else return 1;
    return 0;
}
