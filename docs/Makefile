# German and English online docs:
#   make all
#   make install prefix=/path/to/webspace
# The offline docs:
#   make chm HHC=/path/to/hhc.exe

# Only English online docs, which can be partially updated:
#   make online-en
# The English offline doc:
#   make chm/en/Developer.chm HHC=/path/to/hhc.exe
# XML syntax check:
#   make check

# Extra Parameters for xsltproc can be given in the XSLTFLAGS variable.
# Use prefix to select the directory where the docs are to be installed

prefix = /tmp
HHC = hhc.exe
MKDIR_P = mkdir -p
CP = cp
CP_R = cp -r
NODE = node

stylesheet = clonk.xsl

# Sources:

# find all directories neither beginning nor contained within a directory beginning with a dot
sdk-dirs := $(shell find sdk -name '.*' -prune -o -type d -print)

# find all *.xml files recursively in sdk/
xmlfiles := $(sort $(shell find sdk -name '.*' -prune -o -name 'content.xml' -prune -o -name \*.xml -print))
xmlfiles-de := $(subst sdk, sdk-de, $(xmlfiles))

# misc
extra-files := $(sort $(wildcard *.css *.js images/*.*) lunr.js)
extra-files-chm := $(sort $(wildcard *.css *.js images/*.*))

# Targets:

# strip from all files the .xml, and add a .html
htmlfiles := $(addsuffix .html, $(basename $(xmlfiles)))

# The translated files and directories
sdk-de-dirs := $(subst sdk, sdk-de, $(sdk-dirs))

# For openclonk.org
online-dirs := $(foreach lang, en de, $(addprefix online/$(lang)/, $(sdk-dirs) images))
online-sdk-files := $(foreach lang, en de, $(addprefix online/$(lang)/, $(htmlfiles) sdk/content.html))
online-extra-files := $(foreach lang, en de, $(addprefix online/$(lang)/, $(extra-files)))
online-index-files := $(foreach lang, en de, $(addprefix online/$(lang)/, index.json))

# For Entwickler.chm
chm-dirs := $(foreach lang, en de, $(addprefix chm/$(lang)/, . $(sdk-dirs) images))

.PHONY: all online-en chm install check clean

all: $(sdk-de-dirs) $(online-dirs) $(online-sdk-files) $(online-extra-files) $(online-index-files)

online-en: $(addprefix online/en/, $(sdk-dirs) images $(htmlfiles) sdk/content.html $(extra-files) index.json)

chm: $(sdk-de-dirs) $(chm-dirs) chm/en/Developer.chm chm/de/Entwickler.chm

install: all
	$(MKDIR_P) $(prefix)
	$(CP_R) online/* $(prefix)

check:
	@xmllint --noblanks --noout --valid $(xmlfiles)

clean:
	rm -f *.mo Entwickler.chm Developer.chm doku.pot sdk/content.xml
	rm -rf online sdk-de chm

sdk/content.xml: sdk/content.xml.in $(xmlfiles) tools/build_contents.py tools/experimental.py
	@echo generate $@
	@python2 tools/build_contents.py $(xmlfiles)

# Node dependencies for index.
node_modules/.make: package.json
	npm install
	@touch $@

lunr.js: node_modules/.make
	$(CP) node_modules/lunr/lunr.js $@

online/en/index.json: node_modules/.make $(xmlfiles) tools/build_index.js
	@$(NODE) tools/build_index.js $@ $(xmlfiles)
online/de/index.json: node_modules/.make $(xmlfiles-de) tools/build_index.js
	@$(NODE) tools/build_index.js $@ $(xmlfiles-de)

chm/en/Output.hhp: $(xmlfiles) chm/en/. tools/build_hhp.py Template.hhp
	@echo generate $@
	@python2 tools/build_hhp.py $@ Template.hhp $(xmlfiles)
chm/de/Output.hhp: $(xmlfiles) chm/de/. tools/build_hhp.py Template.de.hhp
	@echo generate $@
	@python2 tools/build_hhp.py $@ Template.de.hhp $(xmlfiles)

$(sdk-de-dirs) $(online-dirs) $(chm-dirs):
	mkdir -p $@

doku.pot: $(xmlfiles) extra-strings.xml sdk/content.xml.in tools/xml2po.py tools/clonk.py
	@echo extract strings to $@
	@python2 tools/xml2po.py -e -m clonk -o $@ $(xmlfiles) extra-strings.xml sdk/content.xml.in

%.po: doku.pot
	@echo update $@
#	@msgmerge --no-wrap -w 1 -U $@ $<
	@msgmerge --no-wrap -w 1 -o $@ $@ $<

%.mo: %.po
	@echo compile $@
	@msgfmt --statistics -o $@ $<

sdk-de/%.xml: sdk/%.xml de.mo tools/xml2po.py tools/clonk.py
	@echo generate $@
	@python2 tools/xml2po.py -e -m clonk -t de.mo -o $@ $<

define run-xslt
@echo generate $@
@xsltproc -o $@ --param chm $(chm) --param fileext "'.html'" $(XSLTFLAGS) $(stylesheet) $<
endef
online/%: chm=0
chm/%: chm=1
online/en/sdk/%.html: sdk/%.xml $(stylesheet) ; $(run-xslt)
online/de/sdk/%.html: sdk-de/%.xml $(stylesheet) ; $(run-xslt)
chm/en/sdk/%.html: sdk/%.xml $(stylesheet) ; $(run-xslt)
chm/de/sdk/%.html: sdk-de/%.xml $(stylesheet) ; $(run-xslt)
chm/en/Output.hhc: stylesheet=hhc.xsl
chm/de/Output.hhc: stylesheet=hhc.xsl
chm/en/Output.hhc: sdk/content.xml hhc.xsl ; $(run-xslt)
chm/de/Output.hhc: sdk-de/content.xml hhc.xsl ; $(run-xslt)
chm/en/Output.hhk: stylesheet=hhk.xsl
chm/de/Output.hhk: stylesheet=hhk.xsl
chm/en/Output.hhk: sdk/content.xml hhk.xsl ; $(run-xslt)
chm/de/Output.hhk: sdk-de/content.xml hhk.xsl ; $(run-xslt)

$(filter online/en/%, $(online-extra-files)): online/en/%: %
	$(CP) $< $@
$(filter online/de/%, $(online-extra-files)): online/de/%: %
	$(CP) $< $@
$(addprefix chm/en/, $(extra-files-chm)): chm/en/%: %
	$(CP) $< $@
$(addprefix chm/de/, $(extra-files-chm)): chm/de/%: %
	$(CP) $< $@

chm/en/Developer.chm: chm/en/Output.hhp chm/en/Output.hhk chm/en/Output.hhc $(addprefix chm/en/, $(sdk-dirs) images $(htmlfiles) $(extra-files-chm))
	! "$(HHC)" $<
chm/de/Entwickler.chm: chm/de/Output.hhp chm/de/Output.hhk chm/de/Output.hhc $(addprefix chm/de/, $(sdk-dirs) images $(htmlfiles) $(extra-files-chm))
	! "$(HHC)" $<

