# TODO

## 2.0

- Extensible `vl::filesystem`.
  - Virtual or filter files/folders (even on root, for wasm or unit test).
  - `vl::filesystem::OSFileSystem` static class to access file system as `Filepath`, `File` and `Folder`.
  - `vl::filesystem::InjectFileSystemImpl(vl::filesystem::IFileSystemImpl*)`, `nullptr` to cancel, a default implementation using `vl::filesystem::OSFileSystem` will take place.
  - Need to affect `FileStream`.
- Base64 read/write from text reader/writer.
  - Implement Base64 encoding as any string types <-> binary.
- Refactor `ITextReader/Writer` to accept all string types.

## Optional
