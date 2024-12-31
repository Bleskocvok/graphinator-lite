# graphinator-lite

Simple utility to display graphs in the terminal. It can display CPU or memory
usage over time (using `monitor`), or it can show a graph from data passed
through to `stdin` (using `gibgraph`).

There are two modes of displaying graphs:

| `fill`                                             | `dots`                                              |
|----------------------------------------------------|-----------------------------------------------------|
| ![terminal screenshot for fill](../demo/fill.png?raw=true) | ![terminal screenshot for dots](../demo/dots.png?raw=true) |

The `fill` option uses symbols that might not be supported in all terminals.
`dots` makes use of braille symbols which should have a wider support.
