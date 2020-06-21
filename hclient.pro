TEMPLATE = app

SOURCES += combobox.cpp\
    hid.cpp\
    hidfinder.cpp\
    hidinfo.cpp\
    list.cpp\
    listbox.cpp\
    main.cpp\
    maindlg.cpp\
    readdlg.cpp\
    writedlg.cpp

HEADERS += combobox.h\
    hclient.h\
    hid.h\
    hidfinder.h\
    hidinfo.h\
    list.h\
    listbox.h\
    maindlg.h\
    readdlg.h\
    resource.h\
    writedlg.h

RC_FILE += hclient.rc
OTHER_FILES += hclient.rc
LIBS += -luser32 -lcomdlg32 -lgdi32 -lhid -lsetupapi
