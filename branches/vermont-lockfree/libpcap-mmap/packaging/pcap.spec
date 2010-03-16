# build libpcap library rpm

%define prefix   /usr
%define word pcap
%define name libpcap
%define version 0.9x.20060222
%define revision 3
%define section 3
%define root /

Summary: packet capture library
Name: %{name}
Version: %{version}
Release: 1
Group: Development/Libraries
Copyright: BSD
Source: %{name}-%{version}.tar.gz
BuildRoot: /tmp/pcap-buildroot
URL: http://public.lanl.gov/cpw

%description
MMAP Packet-capture library %{version}
MMAP version maintained by "Phil Wood"
See http://http://public.lanl.gov/cpw
Please send inquiries/comments/reports to cornett@arpa.net

%prep

%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix --enable-shared
make

%install
mkdir -p $RPM_BUILD_ROOT/usr/{lib,include}
mkdir -p $RPM_BUILD_ROOT/usr/share/{man,doc}
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man3
make install DESTDIR=$RPM_BUILD_ROOT mandir=/usr/share/man
pushd $RPM_BUILD_ROOT/usr/lib
V1=`echo %{version} | sed 's/\\.[^\.]*$//g'`
ln -sf %{name}-$V1.%{revision}.so %{name}.so.$V1
ln -sf %{name}-$V1.%{revision}.so %{name}.so
set `echo $V1 | tr '.' ' '`
V=`expr $2 - 1`
ln -sf %{name}-$V1.%{revision}.so %{name}.so.0.6
ln -sf %{name}-$V1.%{revision}.so %{name}.so.0.7
ln -sf %{name}-$V1.%{revision}.so %{name}.so.0.8
ln -sf %{name}-$V1.%{revision}.so %{name}.so.1.0
ln -sf %{name}-$V1.%{revision}.so %{name}.so.0.8.3
ln -sf %{name}-$V1.%{revision}.so %{name}-0.8.3.so
popd
gzip -c < %{word}.%{section} > $RPM_BUILD_ROOT/usr/share/man/man3/%{word}.%{section}.gz
chmod 644 $RPM_BUILD_ROOT/usr/share/man/man3/%{word}.%{section}.gz
install -m 755 -o root %{name}.a $RPM_BUILD_ROOT/usr/lib
install -m 644 -o root pcap.h $RPM_BUILD_ROOT/usr/include
install -m 644 -o root pcap-bpf.h $RPM_BUILD_ROOT/usr/include
install -m 644 -o root pcap-namedb.h $RPM_BUILD_ROOT/usr/include

%files
%defattr(-,root,root)
%doc LICENSE CHANGES INSTALL.txt README.linux TODO VERSION CREDITS packaging/%{word}.spec

%{root}usr/include/pcap-bpf.h
%{root}usr/include/pcap-namedb.h
%{root}usr/include/pcap.h
%{root}usr/lib/libpcap-0.9.3.so
%{root}usr/lib/libpcap-0.8.3.so
%{root}usr/lib/libpcap.so.0.7
%{root}usr/lib/libpcap.so.0.6
%{root}usr/lib/libpcap.a
%{root}usr/lib/libpcap.la
%{root}usr/share/man/man3/pcap.3.gz

%post
ldconfig

#%clean
#rm -rf $RPM_BUILD_ROOT

%changelog
* Wed Nov 4 2005 Phil Wood <cpw@lanl.gov>
- 2.0-0
* Wed Sep 14 2005 Phil Wood <cpw@lanl.gov>
- 2.0-0
