CXX=g++ -std=c++17 -g
RM=rm -f
RMDIR=rm -rf
HFILES:=$(shell find src/ -type f -iname *.h -print)
CXXFILES:=$(shell find src/ -type f -iname *.cpp -print)
CXXFLAGS=$(shell pkg-config --cflags --libs gtk+-3.0) -rdynamic -export-dynamic -pthread -Wall -lpthread -lgmodule-2.0 -lsteam_api -lcurl -lyajl -ldl
LDFLAGS=-L${CURDIR}/bin
OBJDIR=obj
OBJS=$(addprefix ${OBJDIR}/,$(subst .cpp,.o,${CXXFILES}))

all: ${CURDIR}/bin/samrewritten

${CURDIR}/bin/samrewritten: $(OBJS)
	${CXX} ${CXXFLAGS} ${LDFLAGS} -o ${CURDIR}/bin/samrewritten $(OBJS)

${OBJDIR}/%.o: %.cpp $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) $< ${LDFLAGS} -c -o $@

clean:
	${RMDIR} ${OBJDIR}