CC=gcc
CXX=g++
CXXFLAGS= -g \
		 -lpthread \
		 -Wall \
		 -gstabs
INCPATH = -I. \
		  -I./lib/ \
		  -I../../GTEST/include \
		  -I./small_server/
#all:output/lib_conf_test.o output/lib_conf.o output/lib_log.o output/lib_log_test.o
all:output/lib_conf_test output/lib_log_test output/ss_log_test output/ss_conf_test output/ss_timer_test output/lib_net.o
output/lib_conf_test: output/lib_conf_test.o output/lib_conf.o output/lib_log.o 
	$(CXX) $(INCPATH) $(CXXFLAGS) output/lib_conf_test.o output/lib_conf.o output/lib_log.o -Xlinker "-(" ../../GTEST/lib/gtest_main.a -Xlinker "-)" -o output/lib_conf_test
output/lib_log_test: output/lib_log_test.o output/lib_log.o
	$(CXX) $(INCPATH) $(CXXFLAGS) output/lib_log_test.o output/lib_log.o -Xlinker "-(" ../../GTEST/lib/gtest_main.a -Xlinker "-)" -o output/lib_log_test
output/ss_log_test: output/ss_log_test.o output/ss_log.o output/lib_log.o
	$(CXX) $(INCPATH) $(CXXFLAGS) output/ss_log_test.o output/ss_log.o output/lib_log.o -Xlinker "-(" ../../GTEST/lib/gtest_main.a -Xlinker "-)" -o output/ss_log_test
output/ss_conf_test: output/ss_conf_test.o output/ss_conf.o output/lib_log.o output/lib_conf.o output/ss_log.o
	$(CXX) $(INCPATH) $(CXXFLAGS) output/ss_conf_test.o output/ss_log.o output/ss_conf.o output/lib_log.o output/lib_conf.o -Xlinker "-(" ../../GTEST/lib/gtest_main.a -Xlinker "-)" -o output/ss_conf_test
output/ss_timer_test: output/ss_timer_test.o output/ss_timer.o
	$(CXX) $(INCPATH) $(CXXFLAGS) output/ss_timer_test.o output/ss_timer.o -Xlinker "-(" ../../GTEST/lib/gtest_main.a -Xlinker "-)" -o output/ss_timer_test
output/lib_log.o:lib/lib_log.cpp lib/lib_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) lib/lib_log.cpp -o output/lib_log.o
output/lib_conf.o:lib/lib_conf.cpp lib/lib_conf.h lib/lib_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) lib/lib_conf.cpp -o output/lib_conf.o
output/lib_net.o:lib/lib_net.cpp lib/lib_conf.h lib/lib_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) lib/lib_net.cpp -o output/lib_net.o
output/ss_log.o:small_server/ss_log.cpp small_server/ss_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) small_server/ss_log.cpp -o output/ss_log.o
output/ss_conf.o:small_server/ss_conf.cpp small_server/ss_conf.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) small_server/ss_conf.cpp -o output/ss_conf.o
output/lib_conf_test.o:lib/test/lib_conf_test.cpp lib/lib_conf.h lib/lib_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) lib/test/lib_conf_test.cpp -o output/lib_conf_test.o
output/lib_log_test.o:lib/test/lib_conf_test.cpp lib/lib_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) lib/test/lib_log_test.cpp -o output/lib_log_test.o
output/ss_log_test.o:small_server/test/ss_log_test.cpp small_server/ss_log.h lib/lib_log.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) small_server/test/ss_log_test.cpp -o output/ss_log_test.o
output/ss_conf_test.o:small_server/test/ss_conf_test.cpp small_server/ss_conf.h small_server/ss_log.h lib/lib_log.h lib/lib_conf.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) small_server/test/ss_conf_test.cpp -o output/ss_conf_test.o
output/ss_timer.o:small_server/ss_timer.h small_server/ss_timer.cpp
	$(CXX) -c $(INCPATH) $(CXXFLAGS) small_server/ss_timer.cpp -o output/ss_timer.o
output/ss_timer_test.o:small_server/test/ss_timer_test.cpp small_server/ss_timer.h
	$(CXX) -c $(INCPATH) $(CXXFLAGS) small_server/test/ss_timer_test.cpp -o output/ss_timer_test.o


clean:
	rm -rf output/*
