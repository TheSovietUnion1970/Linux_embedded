Summary: ALSA topology configuration files
Name: alsa-topology-conf
Version: 1.2.5.1
Release: r0
License: BSD-3-Clause
Group: base
Packager: Poky <poky@lists.yoctoproject.org>
URL: https://alsa-project.org

%description
Provides a method for audio drivers to load their mixers, routing, PCMs and
capabilities from user space at runtime without changing any driver source
code.

%package -n alsa-topology-conf-src
Summary: ALSA topology configuration files - Source files
License: BSD-3-Clause
Group: devel

%description -n alsa-topology-conf-src
Provides a method for audio drivers to load their mixers, routing, PCMs and
capabilities from user space at runtime without changing any driver source
code.  This package contains sources for debugging purposes.

%files
%defattr(-,-,-,-)
%dir "/usr"
%dir "/usr/share"
%dir "/usr/share/alsa"
%dir "/usr/share/alsa/topology"
%dir "/usr/share/alsa/topology/bxtrt298"
%dir "/usr/share/alsa/topology/sklrt286"
%dir "/usr/share/alsa/topology/broadwell"
%dir "/usr/share/alsa/topology/hda-dsp"
"/usr/share/alsa/topology/bxtrt298/bxt_i2s.conf"
"/usr/share/alsa/topology/sklrt286/skl_i2s.conf"
"/usr/share/alsa/topology/broadwell/broadwell.conf"
"/usr/share/alsa/topology/hda-dsp/skl_hda_dsp_generic-tplg.conf"

