TEMPLATE = app

SOURCES += button.cpp\
    combobox.cpp\
    element.cpp\
    hid.cpp\
    hidfinder.cpp\
    hidinfo.cpp\
    hiditem.cpp\
    list.cpp\
    listbox.cpp\
    main.cpp\
    maindlg.cpp\
    readdlg.cpp\
    toolbox.cpp\
    writedlg.cpp

HEADERS += button.h\
    combobox.h\
    element.h\
    hclient.h\
    hid.h\
    hidfinder.h\
    hidinfo.h\
    hiditem.h\
    list.h\
    listbox.h\
    maindlg.h\
    readdlg.h\
    resource.h\
    toolbox.h\
    writedlg.h

RC_FILE += hclient.rc
OTHER_FILES += hclient.rc
LIBS += -luser32 -lcomdlg32 -lgdi32 -lhid -lsetupapi
