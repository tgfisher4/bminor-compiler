#!/usr/bin/env bash

outfile="$1"
asm=".tmp.bminor.s"
obj=".tmp.bminor.o"
#bminor_runtime_loc="~tfisher4/pilers-cse40243/compiler"
bminor_runtime_loc="/escnfs/home/tfisher4/pilers-cse40243/compiler"


# Ensure bminor_runtime object exists
cd "$bminor_runtime_loc"
make bminor_runtime
cd -

# Compile assembly to object
gcc -c "$asm" -o "$obj"
# Link object with bminor runtime
# For now, need also to think math since its used in bminor_runtime.
# Ideally, have bminor_runtime library that includes the math library.
gcc "$bminor_runtime_loc"/bminor_runtime.o "$obj" -o "$outfile" -lm

echo "Compilation successful."

# Clean up
rm "$asm" "$obj"
