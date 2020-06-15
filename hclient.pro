TEMPLATE = app

SOURCES += hid.cpp\
    list.cpp\
    main.cpp\
    maindlg.cpp\
    readdlg.cpp\
    writedlg.cpp

HEADERS += hclient.h\
    hid.h\
    list.h\
    maindlg.h\
    readdlg.h\
    resource.h\
    writedlg.h

RC_FILE += hclient.rc
OTHER_FILES += hclient.rc
LIBS += -luser32 -lcomdlg32 -lgdi32 -lhid -lsetupapi
