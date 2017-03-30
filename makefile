all:	ftrest.cpp ftrestd.cpp clean
	g++ -std=c++0x ftrest.cpp -o ftrest
	g++ -std=c++0x ftrestd.cpp -o ftrestd

ftrest: ftrest.cpp
	g++ -std=c++0x ftrest.cpp -o ftrest

ftrestd: ftrestd.cpp
	g++ -std=c++0x ftrestd.cpp -o ftrestd

clean:
	rm -rf ftrest
	rm -rf ftrestd

