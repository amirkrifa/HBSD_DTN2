CPP	:= g++

SRCS	:= ./src/Util.cpp ./src/Bundle.cpp ./src/Bundles.cpp ./src/ConfigFile.cpp ./src/GBOF.cpp ./src/Handlers.cpp ./src/HBSD.cpp ./src/HBSD_Policy.cpp ./src/HBSD_Routing.cpp ./src/HBSD_SAX.cpp ./src/Link.cpp ./src/Links.cpp ./src/Logging.cpp ./src/Node.cpp ./src/Nodes.cpp ./src/PeerListener.cpp ./src/Policy.cpp ./src/Requester.cpp ./src/XMLTree.cpp ./src/Console_Logging.cpp ./src/main.cpp ./src/StatisticsManager.cpp ./src/MeDeHaInterface.cpp

OBJS	:= $(addsuffix .o,$(basename ${SRCS})) 

HEADERS	:= $(addsuffix .h,$(basename ${SRCS})) 

LIBS	:= -lpthread -lxerces-c -L/home/amir/DTN2/xerces-c-src_2_8_0/lib/ -L./src -L/home/amir/xerces-c-src_2_8_0/lib/

OPT	:= -g --verbose -s -Wall 
export CPLUS_INCLUDE_PATH=.:./src:/home/amir/DTN2/xerces-c-src_2_8_0/include/:/home/amir/xerces-c-src_2_8_0/include/


all : HBSD_Router

HBSD_Router: $(OBJS)
	$(CPP) ${INCS} $(OBJS) -o $@ $(LIBS)
	make install

%.o : %.cpp
	${CPP} -c ${OPT} ${INCS} $< -o $@ ${LIBS}

install:
	-mv ./HBSD_Router ./bin/HBSD_Router
	./bin/HBSD_Router -help
clean :
	-rm ./src/*.o ./src/*~
	-clear
 
