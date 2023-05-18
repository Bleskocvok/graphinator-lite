
#include "graph.h"

#include <stdio.h>      // fputs, fscanf, sscanf, stderr, stdin
#include <stdlib.h>     // free, exit, NULL, EXIT_*



int main(int argc, char** argv)
{
    int height = 7;
    double max_value;

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
    double* values = read_input(stdin, &count);

    if (argc >= 3)
    {
        if (sscanf(argv[2], "%lf", &max_value) == 0)
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
