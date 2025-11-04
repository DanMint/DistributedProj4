# DistributedProj4

## Project structure

The project consists of an include folder with all of the header files, a src folder with most of the cpp files (apart from main.cpp which is in the main), the docker compose files, dockerfile, main.cpp, makefile and a README.md.

## How to run:
cd ./project4

make Build-Images

### To run test 1
make Run-Testcase1

### To run test 2
make Run-Testcase2

### To run test 3
make Run-Testcase3

### To run test 4
make Run-Testcase4

### To run test 5
make Run-Testcase5

### Additional Information
Form test cases 3, 4 and 5 I have implemneted my own STORE, RETREIVE and NOT FOUND tests as the docker compose files dont indicate what to do. Hence this function Client::runTestCase(). 