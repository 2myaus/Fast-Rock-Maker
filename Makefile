CXX = g++
CXXFLAGS = -std=c++11 -Wall

TARGET = fastrm
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
