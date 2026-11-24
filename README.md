# popcortex (working name)

A brain made of bits that never stops ticking. Binary spiking neurons on a tiled sheet, wired
locally, learning without backprop, built to suit an ordinary CPU and GPU. Private.

- `docs/design.md`: the mechanisms and how each is laid out for the machine
- `docs/diagram.html`: the same as a picture
- `docs/curriculum.md`: what it is asked to learn, in order
- `docs/plan.md`: the work queue
- `AGENTS.md`: the rules for working here

## Build

```
cmake -S . -B build
nice -n 19 cmake --build build -j4
```

- `build/blobbench`: tick speed
- `build/blobrun`: one blob on one task, headless, score to CSV
- `build/blobview`: the viewer
