#
# spefile for BUTool CLI
#
Name: %{name} 
Version: %{version} 
Release: %{release} 
Packager: %{packager}
Summary: BUTool
License: Apache License
Group: BUTools
Source: https://github.com/BU-Tools/BUTool
URL: https://github.com/BU-Tools/BUTool
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot 
Prefix: %{_prefix}

%description
BUTool CLI Tool

%prep

%build

%install 

# copy includes to RPM_BUILD_ROOT and set aliases
mkdir -p $RPM_BUILD_ROOT%{_prefix}
cp -rp %{sources_dir}/* $RPM_BUILD_ROOT%{_prefix}/.

#Change access rights
chmod -R 755 $RPM_BUILD_ROOT%{_prefix}/lib
chmod -R 755 $RPM_BUILD_ROOT%{_prefix}/bin

%clean 

%post 

%postun 

%files 
%defattr(-, root, root) 
%{_prefix}/lib/*
%{_prefix}/bin/*
%{_prefix}/include/*


