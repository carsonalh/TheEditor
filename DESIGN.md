# TheEditor - Design for v-alpha-0

This is the first "functional" milestone of TheEditor.  It involves:

* having a hideable side panel with a file tree; clicking directories expands/hides them and clicking files opens them in a tab
* having an editor pane. this simply holds the content of the current document, and the ability to edit it; for now, will will only implement a left-right-arrow-controlled cursor with backspace and regular typing
* having a console pane. this will simply have an instance of cmd.exe open, and can use pipes to read/write from it. no console buffers etc.

Thus TheEditor, in terms of bare-bones requirements, will be complete.
