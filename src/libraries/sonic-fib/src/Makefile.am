# --- Jinja2-generated files: header, implementation, and JSON bindings ---

BUILT_SOURCES = src/nexthopgroupfull.h \
                src/nexthopgroupfull.cpp \
                src/nexthopgroupfull_json.h \
                src/c_nexthopgroupfull.h

# Common dependencies
schema_file = $(top_srcdir)/schema/NextHopGroupFull.json
render_script = $(top_srcdir)/scripts/render_schema.py

# Header file for nexthopgroupfull.h
src/nexthopgroupfull.h: $(top_srcdir)/templates/nexthopgroupfull.h.j2 \
                    $(schema_file) \
                    $(render_script)
	$(AM_V_GEN)$(PYTHON) $(render_script) \
		$(schema_file) \
		$(top_srcdir)/templates \
		$@ \
		header

# Implementation file for nexthopgroupfull.cpp
src/nexthopgroupfull.cpp: $(top_srcdir)/templates/nexthopgroupfull.cpp.j2 \
                      $(schema_file) \
                      $(render_script)
	$(AM_V_GEN)$(PYTHON) $(render_script) \
		$(schema_file) \
		$(top_srcdir)/templates \
		$@ \
		source

# JSON bindings inline header file for to_json / from_json handling
src/nexthopgroupfull_json.h: $(top_srcdir)/templates/nexthopgroupfull_json.h.j2 \
                           $(schema_file) \
                           $(render_script)
	$(AM_V_GEN)$(PYTHON) $(render_script) \
		$(schema_file) \
		$(top_srcdir)/templates \
		$@ \
		json_bindings

# C header file for c_nexthopgroupfull.h
src/c_nexthopgroupfull.h: $(top_srcdir)/templates/c_nexthopgroupfull.h.j2 \
                          $(schema_file) \
                          $(render_script)
	$(AM_V_GEN)$(PYTHON) $(render_script) \
		$(schema_file) \
		$(top_srcdir)/templates \
		$@ \
		c_header

CLEANFILES = src/nexthopgroupfull.h \
             src/nexthopgroupfull.cpp \
             src/nexthopgroupfull_json.h \
             src/c_nexthopgroupfull.h

EXTRA_DIST = \
    templates/nexthopgroup/nexthopgroupfull.h.j2 \
    templates/nexthopgroup/nexthopgroupfull.cpp.j2 \
    templates/nexthopgroup/nexthopgroupfull_json.h.j2 \
    templates/nexthopgroup/c_nexthopgroupfull.h.j2 \
    schema/nexthopgroup/NextHopGroupFull.json

# End of Jinja2 generated part

lib_LTLIBRARIES += src/libnexthopgroup.la

src_libnexthopgroup_la_SOURCES = \
    src/nexthopgroupfull.cpp      \
    src/nexthopgroup_debug.cpp    \
    src/c-api/nexthopgroup_capi.cpp

# Install headers to a dedicated subdir
nexthopgroup_headerdir = $(includedir)/nexthopgroup
nexthopgroup_header_HEADERS = \
    src/nexthopgroupfull.h \
    src/nexthopgroupfull_json.h \
    src/nexthopgroup_debug.h \
    src/c_nexthopgroupfull.h

# Install C-API headers
nexthopgroup_capi_headerdir = $(includedir)/nexthopgroup/c-api
nexthopgroup_capi_header_HEADERS = \
    src/c-api/nexthopgroup_capi.h

src_libnexthopgroup_la_CXXFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CODE_COVERAGE_CXXFLAGS)
src_libnexthopgroup_la_CPPFLAGS = $(DBGFLAGS) $(AM_CFLAGS) $(CFLAGS_COMMON) $(CODE_COVERAGE_CPPFLAGS)
src_libnexthopgroup_la_LIBADD = -lpthread
src_libnexthopgroup_la_LDFLAGS = -Wl,-z,now $(LDFLAGS)

