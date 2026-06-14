CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET := cart_pdf_to_excel
SRC := src/main.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) ~/Downloads/yourfilename.pdf
