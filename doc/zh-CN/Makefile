PANDOC ?= pandoc
PDF_ENGINE ?= xelatex
BUILD_DIR ?= build
MANUAL = $(BUILD_DIR)/sdccman-zh-CN
CHAPTERS = \
	chapters/01-introduction.md \
	chapters/02-installing.md \
	chapters/03-using.md \
	chapters/04-language.md \
	chapters/05-targets.md \
	chapters/06-mcs51.md \
	chapters/07-mcs251.md \
	chapters/08-debugging.md \
	chapters/09-tips-support.md \
	chapters/10-technical-internals.md \
	chapters/11-mcs251-abi.md

.PHONY: all pdf html check clean

all: pdf html

pdf: $(MANUAL).pdf

html: $(MANUAL).html

$(BUILD_DIR):
	mkdir -p $@

$(MANUAL).pdf: metadata.yaml $(CHAPTERS) | $(BUILD_DIR)
	$(PANDOC) --metadata-file=metadata.yaml --from=gfm+definition_lists \
		--standalone --table-of-contents --number-sections \
		--pdf-engine=$(PDF_ENGINE) --output=$@ $(CHAPTERS)

$(MANUAL).html: metadata.yaml style.css $(CHAPTERS) | $(BUILD_DIR)
	$(PANDOC) --metadata-file=metadata.yaml --from=gfm+definition_lists \
		--standalone --table-of-contents --number-sections \
		--embed-resources --css=style.css --output=$@ $(CHAPTERS)

check:
	python3 tools/check-manual.py $(CHAPTERS)

clean:
	rm -rf $(BUILD_DIR)
