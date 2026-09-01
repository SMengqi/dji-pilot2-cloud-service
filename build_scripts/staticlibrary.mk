include $(PSROOT)/build_scripts/mk.conf

.PHONY: all clean

all: $(TARGET) $(TARGETSO)

clean:
	$(CLEANLIB)

$(TARGET): $(OBJECT)
	$(MAKELIB)

$(TARGETSO): $(OBJECT)
	$(MAKELIBSO)
	
