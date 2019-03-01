include ../Makefile.bas
A=

all:
	$(A)$(MAKE) -f Makefile.linux ARCH=x64 VERSION=Debug
	$(A)$(MAKE) -f Makefile.linux ARCH=x64 VERSION=Release
	$(A)$(MAKE) -f Makefile.linux ARCH=x86 VERSION=Debug
	$(A)$(MAKE) -f Makefile.linux ARCH=x86 VERSION=Release

clean    :
	$(A)$(MAKE) clean -f Makefile.linux ARCH=x64 VERSION=Debug
	$(A)$(MAKE) clean -f Makefile.linux ARCH=x64 VERSION=Release
	$(A)$(MAKE) clean -f Makefile.linux ARCH=x86 VERSION=Debug
	$(A)$(MAKE) clean -f Makefile.linux ARCH=x86 VERSION=Release

install  :
	$(A)$(MAKE) install -f Makefile.linux ARCH=x64 VERSION=Debug
	$(A)$(MAKE) install -f Makefile.linux ARCH=x64 VERSION=Release
	$(A)$(MAKE) install -f Makefile.linux ARCH=x86 VERSION=Debug
	$(A)$(MAKE) install -f Makefile.linux ARCH=x86 VERSION=Release

##==============================================================================
