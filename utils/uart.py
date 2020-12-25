#!/usr/local/bin/python3

# send binary data file via serial port
# designed for Fuxi SoC
# by MaxXing

import serial
from serial.tools.list_ports import comports

import os
import sys

baudrate = 115200


def get_word(num):
  byte_list = []
  for _ in range(4):
    byte_list.append(int(num & 0xff))
    num >>= 8
  return bytes(byte_list)


def make_packet(file_name, offset, slice_len=1):
  packet = []
  with open(file_name, 'rb') as f:
    size = os.path.getsize(file_name)
    packet.append(get_word(0x9e9e9e9e))
    packet.append(get_word(offset))
    packet.append(get_word(size))
    for _ in range(0, size, slice_len):
      packet.append(f.read(slice_len))
  return packet


def send_uart(ser, packet):
  for i, p in enumerate(packet):
    ser.write(p)
    print('sending {:.2%}...\r'.format(i / len(packet)), end='')
    if ser.in_waiting:
      print(ser.read(ser.in_waiting).decode('utf-8', errors='replace'), end='')
  print()


if sys.platform == 'win32':
  from msvcrt import kbhit as has_char
  from msvcrt import getch as get_char

  def init_tty():
    pass

  def restore_tty(settings):
    pass
else:
  import tty

  def init_tty():
    from termios import tcgetattr
    from tty import setraw
    fd = sys.stdin.fileno()
    settings = tcgetattr(sys.stdin.fileno())
    setraw(fd)
    return settings

  def restore_tty(settings):
    from termios import tcsetattr, TCSADRAIN
    tcsetattr(sys.stdin.fileno(), TCSADRAIN, settings)

  def has_char():
    from select import select
    return select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

  def get_char():
    return sys.stdin.read(1)


def read_uart(ser):
  settings = init_tty()
  try:
    while True:
      # read from UART
      if ser.in_waiting:
        sys.stdout.write(ser.read(ser.in_waiting).decode(
                         'utf-8', errors='replace'))
        sys.stdout.flush()
      # read user input
      if has_char():
        ch = get_char()
        # ^C, just exit
        if ord(ch) == 3:
          print()
          break
        # send to uart
        ser.write(ch)
  except KeyboardInterrupt:
    print()
  finally:
    restore_tty(settings)


if __name__ == '__main__':
  args = sys.argv
  args.pop(0)

  if len(args) < 3:
    print('usage: ./uart.py DEVICE FILE OFFSET')
    print('avaliable devices:')
    for i in comports():
      print(f'  {i.device}')
  else:
    print(args[0])
    ser = serial.Serial(args[0], baudrate, timeout=1)
    packet = make_packet(args[1], int(eval(args[2])))
    send_uart(ser, packet)
    read_uart(ser)
