# CI build-test spec for Fedora/Red Hat infrastructure.
# This spec file is used to validate SDRangel PRs across Fedora versions,
# architectures, and Qt configurations. It is not intended for official
# SDRangel releases or Fedora package submission.

# Build with Qt6 by default; use --without qt6 for a Qt5 build.
%bcond_without qt6

Name:		sdrangel
Version:	7.27.1
Release:	1%{?dist}
Summary:	Software defined radio (SDR) and signal analyzer frontend to various hardware
License:	GPL-3.0-or-later
URL:		https://github.com/f4exb/sdrangel
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

# See https://fedoraproject.org/wiki/Changes/EncourageI686LeafRemoval
ExcludeArch:    %{ix86}

Provides:	bundled(jrtplib) = 3.11.1
Provides:	bundled(qthid)
Provides:	bundled(QtWebApp)

# Build tools
BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  doxygen
BuildRequires:  gcc-c++
BuildRequires:  git-core
BuildRequires:  graphviz
BuildRequires:  libappstream-glib
BuildRequires:  libunwind-devel
BuildRequires:  make
BuildRequires:  pkgconf-pkg-config

# SDR hardware and device libraries
BuildRequires:  airspyone_host-devel
BuildRequires:  hackrf-devel
BuildRequires:  libiio-devel
BuildRequires:  rtl-sdr-devel
BuildRequires:  SoapySDR-devel
BuildRequires:  uhd-devel

# DSP, codecs, and signal processing
BuildRequires:  codec2-devel
BuildRequires:  faad2-devel
BuildRequires:  fftw-devel
BuildRequires:  flac-devel
BuildRequires:  lz4-devel
BuildRequires:  opus-devel
BuildRequires:  opencv-devel
BuildRequires:  rnnoise-devel
BuildRequires:  serialdv-devel
BuildRequires:  zlib-devel

# System and multimedia libraries
BuildRequires:  alsa-lib-devel
BuildRequires:  boost-devel
BuildRequires:  ffmpeg-free-devel
BuildRequires:  hidapi-devel
BuildRequires:  libusbx-devel
BuildRequires:  libxml2-devel
BuildRequires:  pulseaudio-libs-devel

# Packaging / graphics
BuildRequires:  vulkan-headers
Requires:       hicolor-icon-theme

%if %{with qt6}
ExclusiveArch: %{qt6_qtwebengine_arches}
BuildRequires:	qt6-qtbase-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:	cmake(KF6Declarative)
BuildRequires:	cmake(Qt6Charts)
BuildRequires:	cmake(Qt6Core)
BuildRequires:  cmake(Qt6CorePrivate)
BuildRequires:	cmake(Qt6Core5Compat)
BuildRequires:	cmake(Qt6Gui)
BuildRequires:	cmake(Qt6LabsSynchronizer)
BuildRequires:	cmake(Qt6Location)
BuildRequires:	cmake(Qt6Multimedia)
BuildRequires:	cmake(Qt6MultimediaWidgets)
BuildRequires:	cmake(Qt6Network)
BuildRequires:	cmake(Qt6OpenGL)
BuildRequires:	cmake(Qt6OpenGLWidgets)
BuildRequires:	cmake(Qt6Positioning)
BuildRequires:	cmake(Qt6Qml)
BuildRequires:	cmake(Qt6QmlCore)
BuildRequires:	cmake(Qt6QmlNetwork)
BuildRequires:	cmake(Qt6QmlMeta)
BuildRequires:	cmake(Qt6Quick)
BuildRequires:	cmake(Qt6QuickControls2)
BuildRequires:	cmake(Qt6QuickDialogs2)
BuildRequires:	cmake(Qt6QuickLayouts)
BuildRequires:	cmake(Qt6QuickWidgets)
BuildRequires:	cmake(Qt6SerialPort)
BuildRequires:	cmake(Qt6StateMachine)
BuildRequires:	cmake(Qt6Svg)
BuildRequires:	cmake(Qt6SvgWidgets)
BuildRequires:	cmake(Qt6TextToSpeech)
BuildRequires:	cmake(Qt6WebEngineCore)
BuildRequires:	cmake(Qt6WebEngineQuick)
BuildRequires:	cmake(Qt6WebEngineWidgets)
BuildRequires:	cmake(Qt6WebSockets)
BuildRequires:	cmake(Qt6Widgets)
%else
ExclusiveArch:  %{qt5_qtwebengine_arches}
BuildRequires:  gr-osmosdr-devel
BuildRequires:	qt5-qtbase-devel
BuildRequires:	qt5-qtwebsockets-devel
BuildRequires:	qt5-qtwebengine-devel
BuildRequires:	qt5-qtmultimedia-devel
# qtpositioning
BuildRequires:	qt5-qtlocation-devel
BuildRequires:	qt5-qtcharts-devel
BuildRequires:	qt5-qtserialport-devel
BuildRequires:	qt5-qtspeech-devel
BuildRequires:	qt5-qtbase-private-devel
BuildRequires:	qt5-qtgamepad-devel
BuildRequires:	qt5-qtsvg-devel
%endif

%description
SDRangel uses sample source plugins to collect I/Q samples from a hardware
device. Then in the passband returned possibly decimated one or more channel
Rx plugins can be used to demodulate, decode or analyze some part of this
spectrum.

Conversely SDRangel uses sample sink plugins to send I/Q samples to a
hardware device. One or more channel Tx plugins can be used to produce
modulated samples that are mixed into a transmission passband with possible
subsequent interpolation before being sent to the device or written to file.

More information is available on the project Wiki:
https://github.com/f4exb/sdrangel/wiki/Quick-start

%prep
%autosetup -p1 -n sdrangel-v7.27.1

%build
# LIB_SUFFIX workaround for https://github.com/pothosware/SoapyUHD/commit/6b521393cc45c66770f3d4bc69eac7dda982174c.patch
# https://github.com/f4exb/sdrangel/issues/2419
%cmake -DARCH_OPT="" \
    -DDEBUG_OUTPUT=ON \
%if %{with qt6}
    -DENABLE_QT6=ON \
%else
    -DENABLE_QT6=OFF \
%endif
%if "%{?_lib}"=="lib64"
  -DLIB_SUFFIX=64
%endif

%cmake_build

%install
%cmake_install

# drop duplicate readme file, already installed as the doc
rm -f %{buildroot}%{_datadir}/%{name}/Readme.md

%check
desktop-file-validate \
  %{buildroot}%{_datadir}/applications/sdrangel.desktop

%files
%license LICENSE
%doc CHANGELOG Readme.md
%{_bindir}/sdrangel
%{_bindir}/sdrangelbench
%{_bindir}/sdrangelsrv
%{_libdir}/sdrangel
%{_datadir}/applications/sdrangel.desktop
%{_datadir}/icons/hicolor/scalable/apps/sdrangel_icon.svg
%{_metainfodir}/org.sdrangel.SDRangel.metainfo.xml

%changelog
