# Java Classes for EAI/SAI/etc.

# NOTE - automake-1.11.6 changed how class*.stamp is generated, now need
# to use noinst_JAVA and EXTRA_DIST instead of dist_noinst_JAVA to
# keep behaviour consistent and compatible with both automake-1.11.6 and
# older automake

# Get global variables for Makefile defined by toplevel Makefile.globals
include $(top_srcdir)/Makefile.globals

JAVAROOT=.build
CLASSPATH_ENV = CLASSPATH=$(JAVAROOT):$(srcdir):$$CLASSPATH

# ensure JAVAROOT exists, since it is a custom dir
$(JAVAROOT):
	$(MKDIR_P) $@

$(noinst_JAVA): | $(JAVAROOT)

jarfile = vrml.jar
jardir = $(JAVA_DIR)

$(jarfile): $(noinst_JAVA) $(srcdir)/manifest.extras classnoinst.stamp
	$(JAR) cmf $(srcdir)/manifest.extras $@ -C $(JAVAROOT) .

jar_DATA = $(jarfile)	

EXTRA_DIST = $(noinst_JAVA) java.policy manifest.extras
CLEANFILES = $(jarfile)

clean-local:
	-rm -rf $(JAVAROOT)

include Makefile.sources
