RPMBUILD_DIR=${PWD}
PackageName=BUTool
PACKAGE_VER=$(shell git tag -l | tail -1)
PACKAGE_RELEASE=1
Packager = Daniel Gastler

PLATFORM=$(shell uname -p)

rpm: _rpmall
_rpmall: 
	# create the folder structure
	mkdir -p ${RPMBUILD_DIR}/{RPMS/${PLATFORM},SPECS,BUILD,SOURCES,SRPMS}
	mkdir -p ${RPMBUILD_DIR}/SOURCES/bin/tool

	# copy files over
#	find ../include \( -name "*.[hi]" -o -name "*.hpp" -o -name "*.hh" -o -name "*.hxx" \) -exec install -D -m 644 {} ${RPMBUILD_DIR}/SOURCES/include {} \;
	find ../include -exec install -D -m 644 -t ${RPMBUILD_DIR}/SOURCES/include {} \;
	find ../bin     -exec install -D -m 755 -t ${RPMBUILD_DIR}/SOURCES/bin/tool {} \;
	find ../lib     -exec install -D -m 755 -t ${RPMBUILD_DIR}/SOURCES/lib {} \;

	# build the rpm
	rpmbuild -bb -bl --buildroot=${RPMBUILD_DIR}/BUILD						\
			--define "_topdir ${RPMBUILD_DIR}"						\
			--define "_prefix /opt"							\
			--define "sources_dir ${RPMBUILD_DIR}/SOURCES"					\
			--define "name ${PackageName}"								\
			--define "version ${PACKAGE_VER}"	\
			--define "release ${PACKAGE_RELEASE}"							\
			--define "packager ${Packager}"								\
			${PackageName}.spec
#	 find  $(PackagePath)/rpm/RPMBUILD -name "*.rpm" -exec mv {} $(PackagePath)/rpm \;

clean:
	rm -rf ${RPMBUILD_DIR}/{RPMS,SPECS,BUILD,SOURCES,SRPMS}
	rm -rf include bin lib
	rm -rf BUILDROOT RPMS
