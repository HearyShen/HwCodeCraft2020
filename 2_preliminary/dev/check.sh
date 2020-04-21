echo "Compiling..."
g++ -O3 main.cpp -o test -lpthread

echo -e "\ntest on 54:"
./test ../extra_data/54/test_data.txt
diff result.txt ../extra_data/54/result.txt

echo -e "\ntest on 3738:"
./test ../extra_data/3738/test_data.txt
diff result.txt ../extra_data/3738/result.txt

echo -e "\ntest on 38252:"
./test ../extra_data/38252/test_data.txt
diff result.txt ../extra_data/38252/result.txt

echo -e "\ntest on 58254:"
./test ../extra_data/58284/test_data.txt
diff result.txt ../extra_data/58284/result.txt

echo -e "\ntest on 77409:"
./test ../extra_data/77409/test_data.txt
diff result.txt ../extra_data/77409/result.txt
