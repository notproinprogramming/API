CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TARGET = Menu

SOURCES = Menu.cpp Base64.cpp FileUtils.cpp RLE.cpp BitSeq.cpp
OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJECTS) $(TARGET)
