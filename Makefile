CXXFLAGS+=-std=c++17
OBJDIR=obj
LIBDIR?=lib
PREFIX=/usr
HFILES:=$(shell find src/ -type f -iname *.h -print)
CXXFILES:=$(shell find src/ -type f -iname *.cpp -print)
PKG_CONFIG?=pkg-config
GTKFLAGS:=$(shell $(PKG_CONFIG) gtkmm-3.0 --cflags)
GTKLIBS:=$(shell $(PKG_CONFIG) gtkmm-3.0 --libs)
# For now, leave it to the distro to provide preferred extra flags
CXXFLAGS+=$(GTKFLAGS) -Wall
LDLIBS+=$(GTKLIBS) -lsteam_api -lcurl -lyajl -ldl
LDFLAGS+=-L${CURDIR}/bin
OBJS=$(addprefix ${OBJDIR}/,$(subst .cpp,.o,${CXXFILES}))

all: ${CURDIR}/bin/samrewritten
	@printf "==== %b\n" \
		"Use '\033[1mmake dev\033[0m' to keep debug symbols" \
		"Use '\033[1mmake clean\033[0m' to remove object files" \
		"Nothing left to do."

dev: CXXFLAGS += -g -DDEBUG_CERR
dev: ${CURDIR}/bin/samrewritten

.PHONY: clean
clean:
	rm -rf ${OBJDIR}

.PHONY: install
install: bin/launch.sh bin/samrewritten bin/libsteam_api.so
	mkdir -p ${DESTDIR}${PREFIX}/${LIBDIR}/SamRewritten/bin
	mkdir -p ${DESTDIR}${PREFIX}/${LIBDIR}/SamRewritten/glade
	mkdir -p ${DESTDIR}${PREFIX}/${LIBDIR}/SamRewritten/assets
	mkdir -p ${DESTDIR}${PREFIX}/share/icons/hicolor/64x64/apps
	mkdir -p ${DESTDIR}${PREFIX}/share/icons/hicolor/256x256/apps
	mkdir -p ${DESTDIR}${PREFIX}/share/applications
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp $^ ${DESTDIR}${PREFIX}/${LIBDIR}/SamRewritten/bin/
	ln -s ${PREFIX}/${LIBDIR}/SamRewritten/bin/launch.sh ${DESTDIR}${PREFIX}/bin/samrewritten
	cp glade/main_window.glade ${DESTDIR}${PREFIX}/${LIBDIR}/SamRewritten/glade/main_window.glade
	cp assets/icon_64.png ${DESTDIR}${PREFIX}/share/icons/hicolor/64x64/apps/samrewritten.png
	cp assets/icon_256.png ${DESTDIR}${PREFIX}/share/icons/hicolor/256x256/apps/samrewritten.png
	cp assets/icon_256.png ${DESTDIR}${PREFIX}/${LIBDIR}/SamRewritten/assets/
	cp package/samrewritten.desktop ${DESTDIR}${PREFIX}/share/applications/

${CURDIR}/bin/samrewritten: $(OBJS)
	${CXX} -o ${CURDIR}/bin/samrewritten $(OBJS) ${LDFLAGS} ${CXXFLAGS} ${LDLIBS}

${OBJDIR}/%.o: %.cpp $(HFILES)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< ${CXXFLAGS}
