pacspire
========
pacspire is a simple package manager for the TI-Nspire. It allows you to install programs from a zip archive instead of transferring multiple files (e.g. for games) to your calculator. It can also register file extensions at installation time.

todo
----
 - [x] Installing packages
 - [x] Registering file extensions
 - [ ] List packages
 - [ ] Remove packages (can be done manually atm)
 - [ ] tool for PC to autogenerate packages

How to compile
--------------
 - comment out everything zlib-related from `ndless/Ndless-SDK/ndless/include/os.h`
 - type `make` in `libz`
 - type `make` in `minizip`
 - type `make`

How to create a package
-----------------------
create a file called `pkginfo.txt` with the following content:  
```
name=Package name
version=Version string
timestamp=UNIX Timestamp (e.g. 1375988987)
```
if you want, you can add the following lines to register extensions:
```
ext_name=txt
ext_prog=editor
```
you can register as many extensions as you want.

now, create a zip archive with your files and the pkginfo.txt in it (use deflate as compression method), name it *name*.pcs.tns and send it to your calc. After you started pacspire once, you can simply click on the package and it will be installed.
