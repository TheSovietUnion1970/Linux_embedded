Summary: Utilities for managing POSIX Access Control Lists
Name: acl
Version: 2.3.1
Release: r0
License: GPL-2.0-or-later
Group: libs
Packager: Poky <poky@lists.yoctoproject.org>
URL: http://savannah.nongnu.org/projects/acl/
BuildRequires: attr
BuildRequires: autoconf-native
BuildRequires: automake-native
BuildRequires: gettext-native
BuildRequires: libtool-cross
BuildRequires: libtool-native
BuildRequires: virtual/arm-poky-linux-gnueabi-compilerlibs
BuildRequires: virtual/arm-poky-linux-gnueabi-gcc
BuildRequires: virtual/libc
Requires: ld-linux-armhf.so.3
Requires: ld-linux-armhf.so.3(GLIBC_2.4)
Requires: libacl.so.1
Requires: libacl.so.1(ACL_1.0)
Requires: libacl1 >= 2.3.1
Requires: libc.so.6
Requires: libc.so.6(GLIBC_2.33)
Requires: libc.so.6(GLIBC_2.34)
Requires: libc.so.6(GLIBC_2.4)
Requires: libc6 >= 2.35
Requires: rtld(GNU_HASH)

%description
ACL allows you to provide different levels of access to files and folders
for different users.

%package -n acl-src
Summary: Utilities for managing POSIX Access Control Lists - Source files
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: devel

%description -n acl-src
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains sources for debugging purposes.

%package -n acl-dbg
Summary: Utilities for managing POSIX Access Control Lists - Debugging files
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: devel
Recommends: glibc-dbg
Recommends: libacl-dbg
Recommends: libattr-dbg

%description -n acl-dbg
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains ELF symbols and related sources
for debugging purposes.

%package -n libacl1
Summary: Utilities for managing POSIX Access Control Lists
License: LGPL-2.1-or-later
Group: libs
Requires: ld-linux-armhf.so.3
Requires: ld-linux-armhf.so.3(GLIBC_2.4)
Requires: libattr.so.1
Requires: libattr1 >= 2.5.1
Requires: libc.so.6
Requires: libc.so.6(GLIBC_2.33)
Requires: libc.so.6(GLIBC_2.4)
Requires: libc6 >= 2.35
Requires: rtld(GNU_HASH)
Requires(post): ld-linux-armhf.so.3
Requires(post): ld-linux-armhf.so.3(GLIBC_2.4)
Requires(post): libattr.so.1
Requires(post): libattr1 >= 2.5.1
Requires(post): libc.so.6
Requires(post): libc.so.6(GLIBC_2.33)
Requires(post): libc.so.6(GLIBC_2.4)
Requires(post): libc6 >= 2.35
Requires(post): rtld(GNU_HASH)
Provides: libacl = 2.3.1
Provides: libacl.so.1
Provides: libacl.so.1(ACL_1.0)
Provides: libacl.so.1(ACL_1.1)
Provides: libacl.so.1(ACL_1.2)

%description -n libacl1
ACL allows you to provide different levels of access to files and folders
for different users.

%package -n acl-ptest
Summary: Utilities for managing POSIX Access Control Lists - Package test files
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: devel
Requires: /bin/bash
Requires: /bin/sh
Requires: /usr/bin/perl
Requires: acl
Requires: bash
Requires: coreutils
Requires: e2fsprogs-mke2fs
Requires: gawk
Requires: ld-linux-armhf.so.3
Requires: ld-linux-armhf.so.3(GLIBC_2.4)
Requires: libattr.so.1
Requires: libattr1 >= 2.5.1
Requires: libc.so.6
Requires: libc.so.6(GLIBC_2.4)
Requires: libc6 >= 2.35
Requires: make
Requires: perl
Requires: perl-module-constant
Requires: perl-module-cwd
Requires: perl-module-file-basename
Requires: perl-module-file-path
Requires: perl-module-file-spec
Requires: perl-module-filehandle
Requires: perl-module-getopt-std
Requires: perl-module-posix
Requires: rtld(GNU_HASH)
Requires: shadow
Recommends: ptest-runner
Provides: libtestlookup.so.0

%description -n acl-ptest
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains a test directory
/usr/lib/acl/ptest for package test purposes.

%package -n acl-staticdev
Summary: Utilities for managing POSIX Access Control Lists - Development files (Static Libraries)
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: devel
Requires: acl-dev = 2.3.1-r0

%description -n acl-staticdev
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains static libraries for software
development.

%package -n acl-dev
Summary: Utilities for managing POSIX Access Control Lists - Development files
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: devel
Requires: acl = 2.3.1-r0
Requires: libacl
Recommends: attr-dev
Recommends: bash-dev
Recommends: coreutils-dev
Recommends: e2fsprogs-mke2fs-dev
Recommends: gawk-dev
Recommends: glibc-dev
Recommends: libacl-dev
Recommends: libattr-dev
Recommends: make-dev
Recommends: perl-dev
Recommends: perl-module-constant-dev
Recommends: perl-module-cwd-dev
Recommends: perl-module-file-basename-dev
Recommends: perl-module-file-path-dev
Recommends: perl-module-file-spec-dev
Recommends: perl-module-filehandle-dev
Recommends: perl-module-getopt-std-dev
Recommends: perl-module-posix-dev
Recommends: shadow-dev

%description -n acl-dev
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains symbolic links, header files,
and related items necessary for software development.

%package -n acl-doc
Summary: Utilities for managing POSIX Access Control Lists - Documentation files
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: doc

%description -n acl-doc
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains documentation.

%package -n acl-locale-de
Summary: Utilities for managing POSIX Access Control Lists - de translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-de
Provides: acl-locale
Provides: de-translation

%description -n acl-locale-de
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the de locale.

%package -n acl-locale-en+boldquot
Summary: Utilities for managing POSIX Access Control Lists - en@boldquot translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-en+boldquot
Provides: acl-locale
Provides: en+boldquot-translation

%description -n acl-locale-en+boldquot
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the en@boldquot locale.

%package -n acl-locale-en+quot
Summary: Utilities for managing POSIX Access Control Lists - en@quot translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-en+quot
Provides: acl-locale
Provides: en+quot-translation

%description -n acl-locale-en+quot
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the en@quot locale.

%package -n acl-locale-es
Summary: Utilities for managing POSIX Access Control Lists - es translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-es
Provides: acl-locale
Provides: es-translation

%description -n acl-locale-es
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the es locale.

%package -n acl-locale-fr
Summary: Utilities for managing POSIX Access Control Lists - fr translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-fr
Provides: acl-locale
Provides: fr-translation

%description -n acl-locale-fr
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the fr locale.

%package -n acl-locale-gl
Summary: Utilities for managing POSIX Access Control Lists - gl translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-gl
Provides: acl-locale
Provides: gl-translation

%description -n acl-locale-gl
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the gl locale.

%package -n acl-locale-pl
Summary: Utilities for managing POSIX Access Control Lists - pl translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-pl
Provides: acl-locale
Provides: pl-translation

%description -n acl-locale-pl
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the pl locale.

%package -n acl-locale-sv
Summary: Utilities for managing POSIX Access Control Lists - sv translations
License: LGPL-2.1-or-later & GPL-2.0-or-later
Group: libs
Recommends: virtual-locale-sv
Provides: acl-locale
Provides: sv-translation

%description -n acl-locale-sv
ACL allows you to provide different levels of access to files and folders
for different users.  This package contains language translation files for
the sv locale.

%post -n libacl1
# libacl1 - postinst
#!/bin/sh
set -e
if [ x"$D" = "x" ]; then
	if [ -x /sbin/ldconfig ]; then /sbin/ldconfig ; fi
fi


%files
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/bin"
"/usr/bin/chacl"
"/usr/bin/setfacl"
"/usr/bin/getfacl"

%files -n acl-src
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/src"
%dir "/usr/src/debug"
%dir "/usr/src/debug/acl"
%dir "/usr/src/debug/acl/2.3.1-r0"
%dir "/usr/src/debug/acl/2.3.1-r0/build"
%dir "/usr/src/debug/acl/2.3.1-r0/acl-2.3.1"
%dir "/usr/src/debug/acl/2.3.1-r0/build/include"
%dir "/usr/src/debug/acl/2.3.1-r0/build/include/acl"
%dir "/usr/src/debug/acl/2.3.1-r0/build/include/sys"
"/usr/src/debug/acl/2.3.1-r0/build/include/acl/libacl.h"
"/usr/src/debug/acl/2.3.1-r0/build/include/sys/acl.h"
%dir "/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl"
%dir "/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/include"
%dir "/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libmisc"
%dir "/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/test"
%dir "/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_from_mode.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_copy_entry.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_valid.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_error.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_calc_mask.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_entries.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/libobj.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_extended_file_nofollow.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_set_qualifier.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_fd.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_from_xattr.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_size.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_to_any_text.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_dup.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_delete_def_file.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_delete_perm.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_tag_type.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/perm_copy_fd.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_entry.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_perm.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__apply_mask_to_mode.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_to_xattr.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_create_entry.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_set_file.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_check.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_to_any_text.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_extended_file.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_permset.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_file.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_reorder_obj_p.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_copy_int.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_set_permset.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_extended_fd.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/perm_copy_file.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_cmp.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_add_perm.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_extended_file.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_set_tag_type.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_copy_ext.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_from_text.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_extended_file.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_delete_entry.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_init.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_free.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_equiv_mode.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_get_qualifier.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_to_text.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_clear_perms.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_from_xattr.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/libacl.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/acl_set_fd.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__acl_to_xattr.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libacl/__libobj.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/include/acl_ea.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/include/misc.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/include/walk_tree.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libmisc/walk_tree.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libmisc/high_water_alloc.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libmisc/quote.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libmisc/next_line.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/libmisc/unquote.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/test/test_passwd.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/test/test_group.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/do_set.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/sequence.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/user_group.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/getfacl.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/parse.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/setfacl.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/user_group.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/sequence.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/parse.h"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/chacl.c"
"/usr/src/debug/acl/2.3.1-r0/acl-2.3.1/tools/do_set.h"

%files -n acl-dbg
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/lib"
%dir "/usr/bin"
%dir "/usr/lib/.debug"
%dir "/usr/lib/acl"
"/usr/lib/.debug/libacl.so.1.1.2301"
%dir "/usr/lib/acl/ptest"
%dir "/usr/lib/acl/ptest/.libs"
%dir "/usr/lib/acl/ptest/.libs/.debug"
"/usr/lib/acl/ptest/.libs/.debug/libtestlookup.so.0.0.0"
%dir "/usr/bin/.debug"
"/usr/bin/.debug/chacl"
"/usr/bin/.debug/setfacl"
"/usr/bin/.debug/getfacl"

%files -n libacl1
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/lib"
"/usr/lib/libacl.so.1.1.2301"
"/usr/lib/libacl.so.1"

%files -n acl-ptest
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/lib"
%dir "/usr/lib/acl"
%dir "/usr/lib/acl/ptest"
%dir "/usr/lib/acl/ptest/.libs"
%dir "/usr/lib/acl/ptest/test"
%dir "/usr/lib/acl/ptest/build-aux"
"/usr/lib/acl/ptest/Makefile"
"/usr/lib/acl/ptest/run-ptest"
"/usr/lib/acl/ptest/.libs/libtestlookup.so"
"/usr/lib/acl/ptest/.libs/libtestlookup.la"
"/usr/lib/acl/ptest/.libs/libtestlookup.so.0.0.0"
"/usr/lib/acl/ptest/.libs/libtestlookup.so.0"
%dir "/usr/lib/acl/ptest/test/root"
%dir "/usr/lib/acl/ptest/test/nfs"
"/usr/lib/acl/ptest/test/setfacl-X.test"
"/usr/lib/acl/ptest/test/Makemodule.am"
"/usr/lib/acl/ptest/test/runwrapper"
"/usr/lib/acl/ptest/test/sbits-restore.test"
"/usr/lib/acl/ptest/test/test_passwd.c"
"/usr/lib/acl/ptest/test/test.passwd"
"/usr/lib/acl/ptest/test/getfacl-lfs.test"
"/usr/lib/acl/ptest/test/getfacl-noacl.test"
"/usr/lib/acl/ptest/test/malformed-restore-double-owner.acl"
"/usr/lib/acl/ptest/test/misc.test"
"/usr/lib/acl/ptest/test/cp.test"
"/usr/lib/acl/ptest/test/make-tree"
"/usr/lib/acl/ptest/test/run"
"/usr/lib/acl/ptest/test/test.group"
"/usr/lib/acl/ptest/test/utf8-filenames.test"
"/usr/lib/acl/ptest/test/test_group.c"
"/usr/lib/acl/ptest/test/getfacl-recursive.test"
"/usr/lib/acl/ptest/test/sort-getfacl-output"
"/usr/lib/acl/ptest/test/malformed-restore.test"
"/usr/lib/acl/ptest/test/root/restore.test"
"/usr/lib/acl/ptest/test/root/getfacl.test"
"/usr/lib/acl/ptest/test/root/permissions.test"
"/usr/lib/acl/ptest/test/root/setfacl.test"
"/usr/lib/acl/ptest/test/nfs/nfs-dir.test"
"/usr/lib/acl/ptest/test/nfs/nfsacl.test"
"/usr/lib/acl/ptest/build-aux/config.guess"
"/usr/lib/acl/ptest/build-aux/config.sub"
"/usr/lib/acl/ptest/build-aux/test-driver"
"/usr/lib/acl/ptest/build-aux/compile"
"/usr/lib/acl/ptest/build-aux/ltmain.sh"
"/usr/lib/acl/ptest/build-aux/missing"
"/usr/lib/acl/ptest/build-aux/install-sh"
"/usr/lib/acl/ptest/build-aux/depcomp"
"/usr/lib/acl/ptest/build-aux/config.rpath"
"/usr/lib/acl/ptest/build-aux/ar-lib"

%files -n acl-dev
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/include"
%dir "/usr/lib"
%dir "/usr/include/acl"
%dir "/usr/include/sys"
"/usr/include/acl/libacl.h"
"/usr/include/sys/acl.h"
%dir "/usr/lib/pkgconfig"
"/usr/lib/libacl.so"
"/usr/lib/pkgconfig/libacl.pc"

%files -n acl-doc
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/doc"
%dir "/usr/share/man"
%dir "/usr/share/doc/acl"
"/usr/share/doc/acl/COPYING.LGPL"
"/usr/share/doc/acl/CHANGES"
"/usr/share/doc/acl/libacl.txt"
"/usr/share/doc/acl/PORTING"
"/usr/share/doc/acl/extensions.txt"
"/usr/share/doc/acl/COPYING"
%dir "/usr/share/man/man3"
%dir "/usr/share/man/man1"
%dir "/usr/share/man/man5"
"/usr/share/man/man3/acl_valid.3"
"/usr/share/man/man3/acl_create_entry.3"
"/usr/share/man/man3/acl_set_permset.3"
"/usr/share/man/man3/acl_init.3"
"/usr/share/man/man3/acl_error.3"
"/usr/share/man/man3/acl_get_entry.3"
"/usr/share/man/man3/acl_dup.3"
"/usr/share/man/man3/acl_set_qualifier.3"
"/usr/share/man/man3/acl_extended_file_nofollow.3"
"/usr/share/man/man3/acl_delete_def_file.3"
"/usr/share/man/man3/acl_add_perm.3"
"/usr/share/man/man3/acl_extended_fd.3"
"/usr/share/man/man3/acl_get_file.3"
"/usr/share/man/man3/acl_cmp.3"
"/usr/share/man/man3/acl_set_file.3"
"/usr/share/man/man3/acl_entries.3"
"/usr/share/man/man3/acl_clear_perms.3"
"/usr/share/man/man3/acl_from_text.3"
"/usr/share/man/man3/acl_copy_ext.3"
"/usr/share/man/man3/acl_equiv_mode.3"
"/usr/share/man/man3/acl_calc_mask.3"
"/usr/share/man/man3/acl_copy_int.3"
"/usr/share/man/man3/acl_extended_file.3"
"/usr/share/man/man3/acl_get_tag_type.3"
"/usr/share/man/man3/acl_get_fd.3"
"/usr/share/man/man3/acl_to_text.3"
"/usr/share/man/man3/acl_set_tag_type.3"
"/usr/share/man/man3/acl_check.3"
"/usr/share/man/man3/acl_copy_entry.3"
"/usr/share/man/man3/acl_get_perm.3"
"/usr/share/man/man3/acl_to_any_text.3"
"/usr/share/man/man3/acl_set_fd.3"
"/usr/share/man/man3/acl_free.3"
"/usr/share/man/man3/acl_get_permset.3"
"/usr/share/man/man3/acl_delete_perm.3"
"/usr/share/man/man3/acl_from_mode.3"
"/usr/share/man/man3/acl_get_qualifier.3"
"/usr/share/man/man3/acl_size.3"
"/usr/share/man/man3/acl_delete_entry.3"
"/usr/share/man/man1/getfacl.1"
"/usr/share/man/man1/chacl.1"
"/usr/share/man/man1/setfacl.1"
"/usr/share/man/man5/acl.5"

%files -n acl-locale-de
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/de"
%dir "/usr/share/locale/de/LC_MESSAGES"
"/usr/share/locale/de/LC_MESSAGES/acl.mo"

%files -n acl-locale-en+boldquot
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/en@boldquot"
%dir "/usr/share/locale/en@boldquot/LC_MESSAGES"
"/usr/share/locale/en@boldquot/LC_MESSAGES/acl.mo"

%files -n acl-locale-en+quot
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/en@quot"
%dir "/usr/share/locale/en@quot/LC_MESSAGES"
"/usr/share/locale/en@quot/LC_MESSAGES/acl.mo"

%files -n acl-locale-es
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/es"
%dir "/usr/share/locale/es/LC_MESSAGES"
"/usr/share/locale/es/LC_MESSAGES/acl.mo"

%files -n acl-locale-fr
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/fr"
%dir "/usr/share/locale/fr/LC_MESSAGES"
"/usr/share/locale/fr/LC_MESSAGES/acl.mo"

%files -n acl-locale-gl
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/gl"
%dir "/usr/share/locale/gl/LC_MESSAGES"
"/usr/share/locale/gl/LC_MESSAGES/acl.mo"

%files -n acl-locale-pl
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/pl"
%dir "/usr/share/locale/pl/LC_MESSAGES"
"/usr/share/locale/pl/LC_MESSAGES/acl.mo"

%files -n acl-locale-sv
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/locale"
%dir "/usr/share/locale/sv"
%dir "/usr/share/locale/sv/LC_MESSAGES"
"/usr/share/locale/sv/LC_MESSAGES/acl.mo"

