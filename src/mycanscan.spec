Summary: CAN bus data visualizer for the Toyota Prius hybrid car
Name: mycanscan
Version: 10.0
Release: 1
License: GPL
Group: Applications/Communications
URL: http://mycanscan.sourceforge.net
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
mycanscan is a CAN bus data visualizer for the Toyota Prius hybrid
car. It can display much more information than the standard Multi
Function Display. mycanscan is an Open Source project based on the "My
Can Project" by Attila Vass.

%prep
%setup -q

%build
./configure --prefix=%{_prefix} --bindir=%{_bindir} --mandir=%{_mandir}
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/acramr_calc
%{_bindir}/acramr_calc_simple
%{_bindir}/acramr_rev_calc
%{_bindir}/canpoll
%{_bindir}/canterminal
%{_bindir}/cantest
%{_bindir}/canvis
%{_bindir}/cleanfile
%{_bindir}/csv2txt
%{_bindir}/graphcan
%{_bindir}/graphcanz
%doc
