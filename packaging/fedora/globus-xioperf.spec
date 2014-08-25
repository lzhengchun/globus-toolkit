Name:		globus-xioperf
%global _name %(tr - _ <<< %{name})
Version:	4.4
Release:	1%{?dist}
Summary:	Globus Toolkit - XIO Performance Tool

Group:		Applications/Internet
License:	ASL 2.0
URL:		http://www.globus.org/
Source:	http://www.globus.org/ftppub/gt6/packages/globus_xioperf-4.4.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:	globus-common%{?_isa} >= 14
Requires:	globus-xio%{?_isa} >= 3

BuildRequires:	globus-common-devel >= 14
BuildRequires:	globus-xio-devel >= 3
%if %{?fedora}%{!?fedora:0} >= 19 || %{?rhel}%{!?rhel:0} >= 7
BuildRequires:  automake >= 1.11
BuildRequires:  autoconf >= 2.60
BuildRequires:  libtool >= 2.2
%endif
BuildRequires:  pkgconfig

%description
The Globus Toolkit is an open source software toolkit used for building Grid
systems and applications. It is being developed by the Globus Alliance and
many others all over the world. A growing number of projects and companies are
using the Globus Toolkit to unlock the potential of grids for their cause.

The %{name} package contains:
XIO Performance Tool

%prep
%setup -q -n %{_name}-%{version}

%build
%if %{?fedora}%{!?fedora:0} >= 19 || %{?rhel}%{!?rhel:0} >= 7
# Remove files that should be replaced during bootstrap
rm -rf autom4te.cache

autoreconf -if
%endif


%configure \
           --disable-static \
           --docdir=%{_docdir}/%{name}-%{version} \
           --includedir=%{_includedir}/globus \
           --libexecdir=%{_datadir}/globus

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%check
make %{?_smp_mflags} check

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%dir %{_docdir}/%{name}-%{version}
%{_docdir}/%{name}-%{version}/GLOBUS_LICENSE
%{_bindir}/%{name}

%changelog
* Fri Aug 22 2014 Globus Toolkit <support@globus.org> - 4.4-1
- Merge fixes from ellert-globus_6_branch

* Wed Aug 20 2014 Globus Toolkit <support@globus.org> - 4.3-2
- Fix Source path

* Mon Jun 09 2014 Globus Toolkit <support@globus.org> - 4.3-1
- Merge changes from Mattias Ellert

* Fri Apr 18 2014 Globus Toolkit <support@globus.org> - 4.2-1
- Version bump for consistency

* Fri Feb 14 2014 Globus Toolkit <support@globus.org> - 4.1-1
- Packaging fixes

* Thu Jan 23 2014 Globus Toolkit <support@globus.org> - 4.0-1
- Add openssl dependency

* Wed Jun 26 2013 Globus Toolkit <support@globus.org> - 3.1-7
- GT-424: New Fedora Packaging Guideline - no %_isa in BuildRequires

* Wed Mar 13 2013 Globus Toolkit <support@globus.org> - 3.1-6
- More dependencies

* Mon Nov 26 2012 Globus Toolkit <support@globus.org> - 3.1-5
- 5.2.3

* Mon Jul 16 2012 Joseph Bester <bester@mcs.anl.gov> - 3.1-4
- GT 5.2.2 final

* Fri Jun 29 2012 Joseph Bester <bester@mcs.anl.gov> - 3.1-3
- GT 5.2.2 Release

* Wed May 09 2012 Joseph Bester <bester@mcs.anl.gov> - 3.1-2
- RHEL 4 patches

* Tue Feb 14 2012 Joseph Bester <bester@mcs.anl.gov> - 3.1-1
- RIC-229: Clean up GPT metadata

* Mon Dec 05 2011 Joseph Bester <bester@mcs.anl.gov> - 3.0-5
- Update for 5.2.0 release

* Mon Dec 05 2011 Joseph Bester <bester@mcs.anl.gov> - 3.0-4
- Last sync prior to 5.2.0

* Tue Oct 11 2011 Joseph Bester <bester@mcs.anl.gov> - 3.0-3
- Add explicit dependencies on >= 5.2 libraries

* Thu Sep 01 2011 Joseph Bester <bester@mcs.anl.gov> - 3.0-2
- Update for 5.1.2 release

* Wed Dec 08 2010 bester <bester@fedorabox> - 0.2-1
- Autogenerated
