% SIRIKALI(1)
% 
% Feb 2017

NAME
====

sirikali -- manage user encrypted volumes

SYNOPSIS
========

`sirikali` [*options*]

DESCRIPTION
===========

**sirikali** provides a graphical user interface for managing encrypted volumes
for a user.

The actual mounting/encryption process is managed by another service. The
supported services are CryFS, EncFS, GocryptFS, and SecureFS. At least one of
these services must be installed.

Encrypted volumes are created using the "Create Volume" menu. The encrypted
data is stored at the "Volume Path". For normal password protection, use the
"Key" option.

To unmount a volume, click on the volume path and select 'Unmount'.

To mount an existing volume, select "Mount Volume", select the directory containing
the encrypted data, and enter the key information.

Options:

**-d** *dir*
:    Path to where a volume to be auto unlocked/mounted is located.

**-m** *tool*
:    Tool to use to open a default file manager(default tool is xdg-open).

**-e**
:    Start the application without showing the GUI.

**-b** *backend*
:     A name of a backend to retrieve a password from when a volume is open from CLI.
     Supported backends are: "internal","kwallet" and "gnomewallet.
     The first one is always present but the rest are compile time dependencies.

**-k** *val*
:    When opening a volume from CLI,a value of "rw" will open the volume in read\write
     mode and a value of "ro" will open the volume in read only mode.

**-z** *path*
:    Full path of the mount point to be used when the volume is opened from CLI.
     This option is optional.

**-c** *path*
:    Set Volume Configuration File Path when a volume is opened from CLI.

**-i** *min*
:    Set inactivity timeout(in minutes) to dismount the volume when mounted from CLI.

**-h**, **--help**
:   Show a help message.

SEE ALSO
========

cryfs(1), encfs(1), gocryptfs(1)