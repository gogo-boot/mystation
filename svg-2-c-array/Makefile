SIZES := 16 24 32 48 64
VENV := venv
PYTHON := $(VENV)/bin/python3

TARGETS := $(foreach size,$(SIZES),headers-$(size))

.PHONY: all clean $(TARGETS) icons

all: $(TARGETS) icons

$(VENV)/bin/activate:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install --quiet pillow

headers-%: $(VENV)/bin/activate
	PYTHON=$(PYTHON) bash ./svg_to_headers.sh $*

icons: $(TARGETS)
	$(PYTHON) final_generate_icons_h.py

clean:
	rm -rf png
