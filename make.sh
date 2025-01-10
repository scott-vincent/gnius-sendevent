echo Building gnius-sendevent
cd gnius-sendevent
g++ -o gnius-sendevent -I . -I headers \
    gnius-sendevent.cpp \
    -lpthread || exit
strip gnius-sendevent
echo Done
