cmd_Release/obj.target/radio.so := g++ -shared -pthread -rdynamic  -Wl,-soname=radio.so -o Release/obj.target/radio.so -Wl,--whole-archive ./Release/obj.target/radio/src/achronite/ook_send.o ./Release/obj.target/radio/src/achronite/openThings.o ./Release/obj.target/radio/src/energenie/radio.o ./Release/obj.target/radio/src/energenie/hrfm69.o ./Release/obj.target/radio/src/energenie/spis.o ./Release/obj.target/radio/src/energenie/gpio_rpi.o ./Release/obj.target/radio/src/energenie/delay_posix.o -Wl,--no-whole-archive 