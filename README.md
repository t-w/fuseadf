```
      _/_/                                                _/      _/_/   
   _/      _/    _/    _/_/_/    _/_/      _/_/_/    _/_/_/    _/        
_/_/_/_/  _/    _/  _/_/      _/_/_/_/  _/    _/  _/    _/  _/_/_/_/     
 _/      _/    _/      _/_/  _/        _/    _/  _/    _/    _/          
_/        _/_/_/  _/_/_/      _/_/_/    _/_/_/    _/_/_/    _/
```
`fuseadf` is a simple libfuse-based filesystem allowing to mount a volume of
an ADF image (of an Amiga floppy or a hard disk) and to browse/read/write
files and directories it contains.

Volumes (partitions) can be chosen with `-p` option (not needed for images
with one volume only, ie. floppy disk images). By default, it mounts volumes
in read-write mode, for read-only `-o ro` option must be used.

## Usage
```
fuseadf [options] <image_adf> <mount point>
```
where options can be:
- `-p partition` - partition/volume number (0-10), default: 0
- `-l logfile` - enable logging and (optionally) specify logging file,
                   default log file: `fuseadf.log`
- `-i`           - ignore checksum errors
- `-h`           - show help info
- `-V`           - show version

Additionally, some FUSE options can be used (for details see FUSE documentation):
-    `-o mount_options` -  list of mount options (ie. `ro` for read-only mount),
                              see: `man fusermount`
-    `-f`               -  run in foreground (do not daemonize)
-    `-d`               -  run in foreground with more verbose (debug) info
-    `-s`               -  single-threaded (enforced - no need to provide it)

## More info
- Building, testing and installation - see `INSTALL`.
- Authors/contributions - see `AUTHORS`.
- License - see `COPYING`.

## Notes
- It should be considered a (working) prototype, so please beware of,
  for instance, security implications, especially if you get the disk images
  from uncertain sources.

- It relies on [the ADFlib](https://github.com/lclevy/ADFlib) for accessing
  ADF disk images. In particular, for the supported filesystems, check
  information what is supported by the ADFlib. At the moment of writing,
  all classic AmigaDOS filesystems (all versions of OFS and FFS)
  are supported.

- It might not allow to mount images with inconsistent state, in particular
  in read-write mode. In such case, try to mount it read-only (if you only
  need to read it) or repair the image, either on an Amiga emulator with
  native tools, or with some foreign ones (for instance - inconsistency
  in volume's block allocation bitmap can be rebuilt with `adf_bitmap` utility
  from the ADFlib).

- It should be used only in FUSE's single-threaded mode(!). Running in
  multithreaded mode, with multiple processes accessing the filesystem in
  parallel, may result in corrupted data, so beware (esp. when writing data).

  (Since 0.3, the FUSE's single-threaded mode is enforced, adding `-s` is no
  longer needed).


## Related websites and tools:
- ADFlib, a portable library in C (includes some useful tools, eg. `unadf`)
  - [repo](https://github.com/lclevy/ADFlib)
  - [webpage](http://lclevy.free.fr/adflib/)
  - [tech info](http://lclevy.free.fr/adflib/adf_info.html)
- [affs linux kernel driver](https://www.kernel.org/doc/html/v6.9/filesystems/affs.html) (generic, Linux kernel mode way for mounting ADFs, requires root-level access(!))
