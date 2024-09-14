.PHONY: all clean
all:
	gcc gateway.c -o gw -pthread `mysql_config --cflags --libs` -Wall
	gcc sensor.c -o s -pthread
clean:
	rm -rf gw