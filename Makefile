BUILD = .

.PHONY: all
all:
	${CXX} -pthread example.cc -o $(BUILD)/example
