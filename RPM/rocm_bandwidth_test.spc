%define name        rocm_bandwidth_test
%define version     %{getenv:PACKAGE_VER}
%define packageroot %{getenv:PACKAGE_DIR}

Name:       %{name}
Version:    %{version}
Release:    1
Summary:    Test to measure PciE bandwith on ROCm platform

Group:      System Environment/Libraries
License:    Advanced Micro Devices Inc.

%description
This package includes rocm_bandwidth_test, a test that could be
used to measure PciE bandwidth on ROCm platform

%prep
%setup -T -D -c -n %{name}

%install
cp -R %packageroot $RPM_BUILD_ROOT
find $RPM_BUILD_ROOT \! -type d | sed "s|$RPM_BUILD_ROOT||"> test.list

%post
ldconfig

%postun
ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files -f test.list

%defattr(-,root,root,-)
