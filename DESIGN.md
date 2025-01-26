# TheEditor - Design for v-alpha-0

This is the first "functional" milestone of TheEditor.  It involves:

* having a hideable side panel with a file tree; clicking directories expands/hides them and clicking files opens them in a tab
* having an editor pane. this simply holds the content of the current document, and the ability to edit it; for now, will will only implement a left-right-arrow-controlled cursor with backspace and regular typing
* having a console pane. this will simply have an instance of cmd.exe open, and can use pipes to read/write from it. no console buffers etc.

Thus TheEditor, in terms of bare-bones requirements, will be complete.

## Plans for v-alpha-1 and beyond

### Performance

In a release build, CPU usage is about 5% (of total CPU resources; no single thread is being maxxed), and RAM usage is about 50M, with GPU usage approaching 5% of my Intel Xe iGPU.  The combined CPU and GPU usage could indicate that rendering could be expensive.

Our code is only allocating <1MB of RAM, so another 49MB makes no sense.  Potential offenders:

* Visual CRT
* Freetype, unlikely, solution: prepackage 
* System DLLs
* GLFW, least likely

Some ways to test:

* Use the wgl example from glad as a benchmark.
* Try compiling without Visual CRT in release; the cost of re-implementing libc from syscalls cannot be more than a wasted 49MB at runtime! (with /O2 as well).

### Code Structure

Redesigning the container/mask stack is a must.  The current data layout is not very clear, so this should be fixed.

Event handling/propagation should also be implemented.  The container stack can be used for this.  Within one container, it is very acceptable for events to be 'free range'.

Anti aliasing is a feature that could be implemented.
