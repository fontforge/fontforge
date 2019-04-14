.PHONY: deb-src deb-src-control deb rpm-src rpm-src-control

INT_PREFIX:=$(PREFIX)

F_METADATA_REGENERATE?=0
UBUNTU_RELEASE?=
ifneq ($(UBUNTU_RELEASE),)
DEB_VERSION_TRAILER?=0ubuntu1~$(UBUNTU_RELEASE)
DEB_OS_RELEASE?=$(UBUNTU_RELEASE)
else
DEB_VERSION_TRAILER?=
DEB_OS_RELEASE?=
endif
F_PRODUCT_NAME:=$(PACKAGE_NAME)
F_FAKE_PRODUCT_NAME:=fontforge
F_PACKAGE_NAME:=$(F_PRODUCT_NAME)
F_PRODUCT_VERSION:=$(shell git describe)
F_PACKAGE_TRAILER?=
ifneq ($(F_PACKAGE_TRAILER),)
F_PACKAGE_VERSION:=$(F_PRODUCT_VERSION)-$(F_PACKAGE_TRAILER)
else
F_PACKAGE_VERSION:=$(F_PRODUCT_VERSION)
endif
DEB_PACKAGE_VERSION:=$(F_PACKAGE_VERSION)-$(DEB_VERSION_TRAILER)
RPM_PACKAGE_VERSION:=$(shell echo $(F_PACKAGE_VERSION) | sed -e 's/-/./g')
CHANGELOG_TIME:=$(shell date "+%a, %d %b %Y %H:%M:%S")
CHANGELOG_TIMESTAMP:=$(CHANGELOG_TIME) -0500
F_PACKAGER_NAME?=Package Maintainer
F_PACKAGER_MAIL?=releases@fontforge.org

DEB_CHANGELOG_FILE=debian/changelog

debian:
	mkdir -p debian ;

ifeq ($(F_METADATA_REGENERATE),1)
$(DEB_CHANGELOG_FILE): debian
	echo "$(F_PRODUCT_NAME)"" (""$(DEB_PACKAGE_VERSION)"") ""$(DEB_OS_RELEASE)""; urgency=low" > "$(DEB_CHANGELOG_FILE)" ;
	echo "" >> "$(DEB_CHANGELOG_FILE)" ;
	echo "  * Release." >> "$(DEB_CHANGELOG_FILE)" ;
	echo "" >> "$(DEB_CHANGELOG_FILE)" ;
	echo " -- ""$(F_PACKAGER_NAME)"" <""$(F_PACKAGER_MAIL)"">  ""$(CHANGELOG_TIMESTAMP)" >> "$(DEB_CHANGELOG_FILE)" ;

deb-src-control: debian $(DEB_CHANGELOG_FILE)
	if [ "`ls Packaging/debian/m4/* || true`" != "" ] ; then for file in Packaging/debian/m4/* ; do m4 -D "PACKAGE_NAME=$(F_PACKAGE_NAME)" -D "PACKAGE_VERSION=$(DEB_PACKAGE_VERSION)" -D "BINARY=0" -D "PREFIX=$(PREFIX)" < "$$file" > debian/"`basename "$$file"`" ; done ; fi ;
	if [ "`ls Packaging/debian/cp/* || true`" != "" ] ; then for file in Packaging/debian/cp/* ; do cp -pRP "$$file" debian/"`basename "$$file"`" ; done ; fi ;
	if [ "`ls Packaging/debian/cp-src/* || true`" != "" ] ; then for file in Packaging/debian/cp-src/* ; do cp -pRP "$$file" debian/"`basename "$$file"`" ; done ; fi ;

else
deb-src-control:

endif

deb-src: deb-src-control
	cd tests ; $(MAKE) prefetch-fonts ;
	for cfile in config/* m4/* ; do if [ -e "$cfile" ] ; then rm -f "$cfile" ; fi ; done ;
	cd .. ; if [ "$(F_FAKE_PRODUCT_NAME)" != "$(F_PACKAGE_NAME)" ] ; then cp -pRP $(F_FAKE_PRODUCT_NAME) $(F_PACKAGE_NAME) ; fi ; cd $(F_PACKAGE_NAME) ; yes | debuild -S -sa ;

deb: deb-src-control
	cd tests ; $(MAKE) prefetch-fonts ;
	for cfile in config/* ; do if [ -e "$cfile" ] ; then rm -f "$cfile" ; fi ; done ;
	cd .. ; if [ "$(F_FAKE_PRODUCT_NAME)" != "$(F_PACKAGE_NAME)" ] ; then cp -pRP $(F_FAKE_PRODUCT_NAME) $(F_PACKAGE_NAME) ; fi ; cd $(F_PACKAGE_NAME) ; yes | debuild ;

rpm-src-control:
	mkdir -p redhat ;
	for file in Packaging/redhat/m4/* ; do m4 -D "PACKAGE_NAME=$(F_PACKAGE_NAME)" -D "PACKAGE_VERSION=$(RPM_PACKAGE_VERSION)" -D "BINARY=0" -D "PREFIX=/usr" -D "SOURCE_TARBALL_NAME=$(F_PACKAGE_NAME)-$(RPM_PACKAGE_VERSION).tar.gz" < "$$file" > redhat/"`basename "$$file"`" ; done ;

rpm-src: rpm-src-control
	cd tests ; $(MAKE) prefetch-fonts ;
	for cfile in config/* m4/* ; do if [ -e "$cfile" ] ; then rm -f "$cfile" ; fi ; done ;
	cd .. ; cp -pRP $(F_FAKE_PRODUCT_NAME)/redhat/rpm.spec ./$(F_PACKAGE_NAME)-$(RPM_PACKAGE_VERSION).spec ; if [ "$(F_FAKE_PRODUCT_NAME)" != "$(F_PACKAGE_NAME)-$(F_PACKAGE_VERSION)" ] ; then cp -pRP $(F_FAKE_PRODUCT_NAME) $(F_PACKAGE_NAME)-$(RPM_PACKAGE_VERSION) ; fi ; tar -czf $(F_PACKAGE_NAME)-$(RPM_PACKAGE_VERSION).tar.gz $(F_PACKAGE_NAME)-$(RPM_PACKAGE_VERSION) ;


