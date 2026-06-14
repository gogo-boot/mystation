SIZES := 24 32 48 64
VENV := venv
PYTHON := $(VENV)/bin/python3

.PHONY: all clean icons

all: $(VENV)/bin/activate
	rm -rf png
	@for size in $(SIZES); do \
		PYTHON=$(PYTHON) bash ./svg_to_headers.sh $$size; \
	done
	$(PYTHON) final_generate_icons_h.py

$(VENV)/bin/activate:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install --quiet pillow

icons: $(VENV)/bin/activate
	$(PYTHON) final_generate_icons_h.py

clean:
	rm -rf png $(VENV)
