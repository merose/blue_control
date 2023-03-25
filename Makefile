LDFLAGS=-lrobotcontrol -lczmq
CFLAGS=-Iinclude

PGMS = \
	bin/json_example \
	bin/sweep_servo \
	bin/read_trk \
	bin/read_gpio \
	bin/json_example \
        bin/json_echo \
        bin/echo_server \
	bin/get_system_params \
	bin/test_pid \
	bin/mobility_server

bin/%: src/%.c
	cc ${CFLAGS} -o $@ $< ${LDFLAGS} 

bin/%: src/%.cpp
	c++ ${CFLAGS} -o $@ $< ${LDFLAGS}

all: ${PGMS}

clean:
	rm ${PGMS}