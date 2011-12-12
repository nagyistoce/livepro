
win32 {
	VPATH += $$PWD
	DEPENDPATH += $$PWD

	INCLUDEPATH += \
		$$PWD/include/msinttypes \
		$$PWD/include/libswscale \
		$$PWD/include/libavutil \
		$$PWD/include/libavdevice \
		$$PWD/include/libavformat \
		$$PWD/include/libavcodec \
		$$PWD/include
	
	LIBS += -L"$$PWD/lib" \
		-lavcodec-51 \
		-lavformat-52 \
		-lavutil-49 \
		-lavdevice-52 \
		-lswscale-0
}
