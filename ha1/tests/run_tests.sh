#!/usr/bin/sh

echo "running tests..."

# big one
for i in `seq 1 1 64`; do python3 ./generator.py -f data$i.txt -c 20050 -m 65536; done
../run $(ls -1 data*.txt | tr "\n" " ") 1>/dev/null
echo "test 1 (large input):"
python3 checker.py -f sequence.txt
rm *.txt

# different sizes
python3 generator.py -f data1.txt -c 16384 -m 16384
python3 generator.py -f data1.txt -c 8192 -m 8192
python3 generator.py -f data1.txt -c 4096 -m 4096
../run $(ls -1 data*.txt | tr "\n" " ") 1>/dev/null
echo "test2 (different file sizes):"
python3 checker.py -f sequence.txt
rm *.txt

# one file
python3 generator.py -f data1.txt -c 4096 -m 4096
../run $(ls -1 data*.txt | tr "\n" " ") 1>/dev/null
echo "test 3 (one input file):"
python3 checker.py -f sequence.txt
rm *.txt

# empty input (expecting usage)
echo "test4 (empty input, expecting usage):"
../run
