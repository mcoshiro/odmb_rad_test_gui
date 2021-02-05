

all: odmb_rad_test_sw

odmb_rad_test_sw: src/odmb_rad_test_sw.cxx
	i686-w64-mingw32-g++ -static src/odmb_rad_test_sw.cxx -o bin/odmb_rad_test_sw.exe -lwinmm -lgdi32
