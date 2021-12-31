# flake
Rewrite of GNU Make with better error handling and Windows support and stuff

Probably would have been a better idea to write it in Python

### Goals:
- Good error handling (normal human readable errors)
- Basic optional Windows support (replace cp with copy)
- Add quality of life upgrades while keeping compatibility with GNU make

### Implementation:
- [x] Basic rules
- [ ] basic prerequisities
- [x] Compile itself
- [x] Macros
- [ ] Basic targets
- [ ] wildcard targets (have `wldCmp`)
- [ ] Include
- [ ] Functions

### Building:
```
cc main.c -o /bin/flake
flake all
```
