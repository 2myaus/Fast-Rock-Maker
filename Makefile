CXX = g++
CXXFLAGS = -std=c++11 -Wall

OPTIMIZE_FLAGS = -Ofast -ffast-math -march=native -funroll-loops -finline-functions -flto
CXXFLAGS += $(OPTIMIZE_FLAGS)

TARGET = fastrm
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

image: $(TARGET)
	./$(TARGET)
	ffmpeg -y -i heatmap.ppm heatmap.png

optimize: clean
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
