# Web Communication Protocal (UDP or TCP)

There are two types of packets, motion packets and macro
packets. Motion packets are meant to be sent in a constant stream to manually control the robot (drive motor and actuator control data). Macro packets are meant to be sent once to make the robot do a predefined action specified below

## Motion Packets

## Macro Packets

#### Packet Structure
1/2 byte packets  
<byte 1> <byte 2>  
byte 1: 
- [0]: Always 1 to represent macro packet flag
- [1..6]: Exclusive macro code to represent action to execute
- [7]: Checksum (0 if odd number of ones, 1 if even)

byte 2: OPTIONAL data argument  

Macro Codes  

Decimal | Binary  | Action                 | Data Required
--------|---------|------------------------|--------------
0       | 000 000 | ESTOP                  | No
1       | 000 001 | Cancel macro           | No
2       | 000 010 | Full extend actuators  | No
3       | 000 011 | Full retract actuators | No
4       | 000 100 | Carry position         | No
5       | 000 101 | Dump cycle             | No
6       | 000 110 | Dump cycle w/ movement | No
7       | 000 111 | Dig cycle              | No
8       | 001 000 | Turn right 45 deg.     | No
9       | 001 001 | Turn left 45 deg.      | No
10      | 001 010 | Turn                   | Yes
11      | 001 011 |
12      | 001 100 |
13      | 001 101 |
14      | 001 110 |
15      | 001 111 |
16      | 010 000 |
17      | 010 001 |
18      | 010 010 |
19      | 010 011 |
20      | 010 100 |
21      | 010 101 |
22      | 010 110 |
23      | 010 111 |
24      | 011 000 |
25      | 011 001 |
26      | 011 010 |
27      | 011 011 |
28      | 011 100 |
29      | 011 101 |
30      | 011 110 |
31      | 011 111 |
32      | 100 000 |
33      | 100 001 |
34      | 100 010 |
35      | 100 011 |
36      | 100 100 |
37      | 100 101 |
38      | 100 110 |
39      | 100 111 |
40      | 101 000 |
41      | 101 001 |
42      | 101 010 |
43      | 101 011 |
44      | 101 100 |
45      | 101 101 |
46      | 101 110 |
47      | 101 111 |
48      | 110 000 |
49      | 110 001 |
50      | 110 010 |
51      | 110 011 |
52      | 110 100 |
53      | 110 101 |
54      | 110 110 |
55      | 110 111 |
56      | 111 000 |
57      | 111 001 |
58      | 111 010 |
59      | 111 011 |
60      | 111 100 |
61      | 111 101 |
62      | 111 110 |
63      | 111 111 |