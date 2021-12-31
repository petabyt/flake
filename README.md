# flake
Rewrite of GNU Make with better error handling and Windows support and stuff

### Goals:
- Good error handling (normal human readable errors)
- Basic optional Windows support (replace cp with copy)
- Add quality of life upgrades while keeping compatibility with GNU make

### Implementation:
- [x] Basic targets
- [ ] prerequisities
- [x] Compile itself
- [x] Macros
- [ ] Link targets to files and modification dates
- [ ] wildcard/multifile targets
- [ ] Include
- [ ] Functions

### Building:
```
cc main.c -o /bin/flake
flake all
```
