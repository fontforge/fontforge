.PHONY: deb-src deb-src-control deb rpm-src rpm-src-control

INT_PREFIX:=$(PREFIX)

F_METADATA_REGENERATE?=0
DEB_VERSION_TRAILER:=
ifneq ($(UBUNTU_RELEASE),)
DEB_VERSION_TRAILER:=0ubuntu1~$(UBUNTU_RELEASE)
DEB_OS_RELEASE:=$(UBUNTU_RELEASE)
endif
DEB_OS_RELEASE?=
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
	cd .. ; if [ "$(F_FAKE_PRODUCT_NAME)" != "$(F_PACKAGE_NAME)" ] ; then cp -pRP $(F_FAKE_PRODUCT_NAME) $(F_PACKAGE_NAME) ; fi ; cd $(F_PACKAGE_NAME) ; yes | debuild -S -sa ;

deb: deb-src-control
	cd .. ; if [ "$(F_FAKE_PRODUCT_NAME)" != "$(F_PACKAGE_NAME)" ] ; then cp -pRP $(F_FAKE_PRODUCT_NAME) $(F_PACKAGE_NAME) ; fi ; cd $(F_PACKAGE_NAME) ; yes | debuild ;

rpm-src-control:
	mkdir -p redhat ;
	for file in packaging/redhat/m4/* ; do m4 -D "PACKAGE_NAME=$(PACKAGE_NAME)" -D "PACKAGE_VERSION=$(PACKAGE_VERSION)" -D "BINARY=0" -D "CLIENT=1" -D "SERVER=0" -D "PREFIX=$(PREFIX)" < "$$file" > redhat/"`basename "$$file"`" ; done ;

rpm-src: rpm-src-control
	cd .. ; cp -pRP $(F_FAKE_PRODUCT_NAME)/redhat/rpm.spec ./$(F_PACKAGE_NAME)-$(F_PACKAGE_VERSION).spec ; if [ "$(F_FAKE_PRODUCT_NAME)" != "$(F_PACKAGE_NAME)" ] ; then cp -pRP $(F_FAKE_PRODUCT_NAME) $(PACKAGE_NAME)-$(PACKAGE_VERSION) ; fi ; tar -czf $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(PACKAGE_NAME)-$(PACKAGE_VERSION) ;


