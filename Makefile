CXX = g++
CXXFLAGS = -std=c++11 -Wall

TARGET = fastrm
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

image: $(TARGET)
	./$(TARGET)
	ffmpeg -y -i heatmap.ppm heatmap.png

clean:
	rm -f $(TARGET)