// Emscripten 3.1.70's FS.rename() leaves a file it replaces in the table it looks names up in, so after a rename over
// a file, as QSaveFile does, removing that name takes off only the newer file and the replaced one is found again.
(() => {
  const rename = FS.rename;
  FS.rename = (oldPath, newPath) => {
    let replaced = null;
    try {
      replaced = FS.lookupPath(newPath).node;
    } catch (e) {
      // Nothing there to replace
    }
    rename(oldPath, newPath);
    if (replaced && FS.lookupPath(newPath).node !== replaced) {
      FS.hashRemoveNode(replaced);
    }
  };
})();
