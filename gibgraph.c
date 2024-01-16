
#include "graph.h"

#include <stdio.h>      // fputs, fscanf, sscanf, stderr, stdin
#include <stdlib.h>     // free, exit, NULL, EXIT_*


void usage(char** argv)
{
    fprintf(stderr, "Usage: %s ‹dots | fill› ‹HEIGHT› [MAX_VALUE]\n", argv[0]);
}

int main(int argc, char** argv)
{
    int height = 7;
    double max_value;
    style_t style;

    if (argc >= 3)
    {
        if (style_from_str(argv[1], &style))
            return fputs("ERROR: invalid style\n", stderr), usage(argv),
                   EXIT_FAILURE;

        if (sscanf(argv[2], "%d", &height) == 0)
            return fputs("ERROR: invalid height\n", stderr), usage(argv),
                   EXIT_FAILURE;
    }
    else
        return usage(argv), EXIT_FAILURE;

    int count = 0;
    double* values = read_input(stdin, &count);

    if (argc >= 4)
    {
        if (sscanf(argv[2], "%lf", &max_value) == 0)
            return fputs("ERROR: invalid max_value\n", stderr), usage(argv),
                   EXIT_FAILURE;
    }
    else
    {
        max_value = count > 0 ? values[0] : 0;
        for (int i = 1; i < count; ++i)
            max_value = values[i] > max_value ? values[i] : max_value;
    }

    render_graph(values, count, max_value, height, NULL, style);

    free(values);

    return EXIT_SUCCESS;
}
