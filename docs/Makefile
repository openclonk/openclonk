# Notable targets:
# all (default): German and English onlinedocs
# online-de: only German onlinedocs, which can be partially updated
# Enwickler.chm: the German offline doc
#                use with make Entwickler.chm HHC = /path/to/hhc

# Extra Parameters for xsltproc can be given in the XSLTFLAGS variable.
# Use prefix to select the directory where the docs are to be installed

prefix = /tmp
HHC = hhc.exe
MKDIR_P = mkdir -p
CP = cp
CP_R = cp -r

stylesheet = clonk.xsl

# Sources:

# find all directories neither beginning nor contained within a directory beginning with a dot
sdk-dirs := $(shell find sdk -name '.*' -prune -o -type d -print)

# misc
extra-files := $(sort $(wildcard *.css *.php *.js images/*.*))
extra-files-chm := $(sort $(wildcard *.css *.js images/*.*))

# find all *.xml files recursively in sdk/
xmlfiles := $(sort $(shell find sdk -name '.*' -prune -o -name \*.xml -print))

# Targets:

# strip from all files the .xml, and add a .html
htmlfiles := $(addsuffix .html, $(basename $(xmlfiles)))

# The translated files and directories
sdk-dirs-en := $(subst sdk, sdk-en, $(sdk-dirs))
xmlfiles-en := $(subst sdk, sdk-en, $(xmlfiles))
htmlfiles-en := $(subst sdk, sdk-en, $(htmlfiles))

# For clonk.de
online-sdk-files := $(foreach lang, en de, $(addprefix online/$(lang)/, $(htmlfiles) content.html))
online-dirs := $(foreach lang, en de, $(addprefix online/$(lang)/, $(sdk-dirs) images))
online-extra-files := $(foreach lang, en de, $(addprefix online/$(lang)/, $(extra-files)))

# For Entwickler.chm
chm-dirs := $(foreach lang, en de, $(addprefix chm/$(lang)/, . $(sdk-dirs) images))

.PHONY: all online-de chm install svn-update check clean

all: $(online-dirs) $(sdk-dirs-en) $(online-extra-files) $(online-sdk-files)

online-de: $(addprefix online/de/, $(sdk-dirs) images $(htmlfiles) $(extra-files))

chm: $(chm-dirs) Entwickler.chm Developer.chm

install: all
	$(MKDIR_P) $(prefix)
	$(CP_R) $(PWD)/online/* $(prefix)

svn-update:
	svn up

check:
	xmllint --noblanks --noout --valid $(xmlfiles)

clean:
	rm -f *.mo Entwickler.chm Developer.chm doku.pot
	rm -rf online sdk-en chm

chm/de/Output.hhp chm/de/Output.hhk chm/en/Output.hhc chm/en/Output.hhp chm/en/Output.hhk: chm/de/Output.hhc
#update timestamp
	touch $@
chm/de/Output.hhc: $(xmlfiles) chm/de/. chm/en/. build_chm_files.py experimental.py \
  Template.hhc Template.en.hhc Template.hhk Template.en.hhk Template.hhp Template.en.hhp en.mo
	@echo generate chm files
	@python build_chm_files.py $(xmlfiles)

online/de/content.html: chm/de/Output.hhc build_contents.pl
	@echo generate $@
	@perl build_contents.pl $< > $@
online/en/content.html: chm/en/Output.hhc build_contents.pl
	@echo generate $@
	@perl build_contents.pl $< > $@

$(sdk-dirs-en) $(online-dirs) $(chm-dirs):
	mkdir -p $@

doku.pot: $(xmlfiles) extra-strings.xml xml2po.py clonk.py
	@echo extract strings to $@
	@python xml2po.py -e -m clonk -o $@ $(xmlfiles) extra-strings.xml

%.po: doku.pot
	@echo update $@
	@msgmerge --no-wrap -w 1 -U $@ $<

%.mo: %.po
	@echo compile $@
	@msgfmt --statistics -o $@ $<

sdk-en/%.xml: sdk/%.xml en.mo xml2po.py clonk.py
	@echo generate $@
	@python xml2po.py -e -m clonk -t en.mo -o $@ $<

define run-xslt
@echo generate $@
@xsltproc -o $@ --param webnotes $(webnotes) --param fileext "'.html'" $(XSLTFLAGS) $(stylesheet) $<
endef
online/%: webnotes=1
chm/%: webnotes=0
online/de/sdk/%.html: sdk/%.xml $(stylesheet) ; $(run-xslt)
online/en/sdk/%.html: sdk-en/%.xml $(stylesheet) ; $(run-xslt)
chm/de/sdk/%.html: sdk/%.xml $(stylesheet) ; $(run-xslt)
chm/en/sdk/%.html: sdk-en/%.xml $(stylesheet) ; $(run-xslt)

$(filter online/en/%, $(online-extra-files)): online/en/%: %
	$(CP) $< $@
$(filter online/de/%, $(online-extra-files)): online/de/%: %
	$(CP) $< $@
$(addprefix chm/en/, $(extra-files-chm)): chm/en/%: %
	$(CP) $< $@
$(addprefix chm/de/, $(extra-files-chm)): chm/de/%: %
	$(CP) $< $@

Entwickler.chm: chm/de/Output.hhp chm/de/Output.hhk chm/de/Output.hhc $(addprefix chm/de/, $(sdk-dirs) images $(htmlfiles) $(extra-files-chm))
	$(HHC) $<
Developer.chm: chm/en/Output.hhp chm/en/Output.hhk chm/en/Output.hhc $(addprefix chm/en/, $(sdk-dirs) images $(htmlfiles) $(extra-files-chm))
	$(HHC) $<

