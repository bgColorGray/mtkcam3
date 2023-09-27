.phony: all clean

ENV:=LANG=C
CPPFLAGS:=-DP2G_ENGINE_UT
CFLAGS:=-std=c++11 -g
CC=g++ $(CPPFLAGS) $(CFLAGS)

exe:=test_p2a
header:=*.h ../*.h
core:=core

src:=UT_Main.cpp
src+=UT_FakePool.cpp
src+=UT_LogUtil.cpp
src+=UT_P2ATestCase.cpp
src+=UT_P2ATestCaseBasic.cpp
src+=UT_P2ATestCaseCheckValidateSanity.cpp
src+=UT_P2ATestCaseDSDN.cpp
src+=UT_P2ATestCaseDualCam.cpp
src+=UT_P2ATestCaseSMVR.cpp
src+=UT_P2ATestTool.cpp
src+=UT_P2AUtil.cpp
src+=UT_P2AValidate.cpp
src+=UT_TypeWrapper.cpp

src+=../P2GDefine.cpp
src+=../PathEngine.cpp

obj:=$(patsubst %.cpp, %.o, $(src))
log:=log

all:init $(exe)

init:
	$(ENV) rm -f $(log)
	clear

$(exe):$(obj) $(header)
	@echo $(ENV) $(CC) -o $(exe) $(obj) >> $(log)
	$(ENV) $(CC) -o $(exe) $(obj) 2>&1 | tee -a $(log)

$(obj):$(header)

%.o:%.cpp
	@echo $(ENV) $(CC) -c $(filter %.cpp, $^) -o $@ >> $(log)
	$(ENV) $(CC) -c $(filter %.cpp, $^) -o $@ 2>&1 | tee -a $(log)

clean:
	clear
	$(ENV) rm -f $(exe) $(obj) $(log) $(core)
