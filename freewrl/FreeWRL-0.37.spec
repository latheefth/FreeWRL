Summary: A vrml viewer
Name: freewrl
Version: 0.37
Release: 1
Copyright: GPL
Group: Applications/Multimedia
Source: FreeWRL-0.37.tar.gz
Patch: none

%description
FreeWRL is an Open Source, cross platform VRML2 compliant browser, with script and EAI support. The primary
platform is Linux; used by FreeWRL users. A Windows port is not yet started, but will eventually happen. Free

%prep
%setup -q

%build
perl Makefile.PL
make 

%install
make install

%clean

%files
%defattr(-,root,root)

/usr/bin/freewrl
/usr/share/man/man1/freewrl.1
/usr/share/man/man3/VRML::Browser.3pm
/usr/share/man/man3/VRML::Viewer.3pm
/usr/lib/perl5/site_perl/5.6.1/i386-linux/VRML
/usr/lib/perl5/site_perl/5.6.1/i386-linux/auto/VRML
/usr/lib/mozilla-1.0.1/vrml.jar
/usr/lib/mozilla-1.0.1/plugins/npfreewrl.so
