main: main.cpp
	g++ main.cpp -o main -std=c++17 -Wall -pedantic #-Werror

graph-only:
	gnuplot -e "set terminal png size 1280,720; \
			set output 'graph.png'; \
			plot 'data.dat' using 1:2 title 'Total sick cases' with lines linestyle 1,\
				'' using 1:3 title 'Total dead' with lines linestyle 2,\
				'' using 1:4 title 'Healthy' with lines linestyle 3,\
				'' using 1:5 title 'Asymptomatic' with lines linestyle 4,\
				'' using 1:6 title 'Mildly symptomatic' with lines linestyle 5,\
				'' using 1:7 title 'Severely symptomatic' with lines linestyle 6";


graph: data.dat
	gnuplot -e "set terminal png size 1280,720; \
			set output 'graph.png'; \
			plot 'data.dat' using 1:2 title 'Total sick cases' with lines linestyle 1,\
				'' using 1:3 title 'Total dead' with lines linestyle 2,\
				'' using 1:4 title 'Healthy' with lines linestyle 3,\
				'' using 1:5 title 'Asymptomatic' with lines linestyle 4,\
				'' using 1:6 title 'Mildly symptomatic' with lines linestyle 5,\
				'' using 1:7 title 'Severely symptomatic' with lines linestyle 6";

data.dat: main
	./main

clean:
	rm main;
	rm data.dat; 
	rm graph.png