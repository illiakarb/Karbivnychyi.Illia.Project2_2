make:
	g++ -std=c++11 src/main.cpp src/tga.cpp -o project2.out

tasks:
	./project2.out output/part11.tga input/circles.tga multiply input/layer1.tga
	./project2.out output/part12.tga input/layer1.tga flip
	./project2.out output/part13.tga input/layer1.tga subtract input/layer2.tga
