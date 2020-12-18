#!/usr/local/bin/python3

# convert binary data file to coe file
# designed for Fuxi SoC
# by MaxXing

import sys


def read_bin(file_name):
  words = []
  with open(file_name, 'rb') as f:
    while True:
      x = f.read(4)
      if x == b'':
        break
      word = int.from_bytes(x, byteorder='little')
      words.append(hex(word)[2:].zfill(8))
  return words


def make_coe(words):
  coe = 'memory_initialization_radix = 16;\n'
  coe += 'memory_initialization_vector =\n'
  contents = ['00000000'] * 128 + words
  coe += '\n'.join(contents) + '\n'
  return coe


if __name__ == '__main__':
  if len(sys.argv) < 2:
    print('usage: ./bin2coe.py FILE')
    exit(1)
  
  bin_file = sys.argv[1]
  coe_file = f'{bin_file}.coe'

  words = read_bin(bin_file)
  coe = make_coe(words)
  
  with open(coe_file, 'w') as f:
    f.write(coe)
